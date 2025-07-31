export module SimpleEngine.Core:ECS.EntityManager;
import :ECS.Entity;

import SimpleEngine.Types;
import std;

import <cassert>;


namespace se::core::ecs
{
export class EntityManager
{
public:
    explicit EntityManager(uint32 in_max_entities)
        : entity_records(in_max_entities)
        , max_entities(in_max_entities)
    {
    }

    Entity Create()
    {
        uint32 id;

        // 재활용 가능한 ID 있으면 우선 사용
        if (!free_ids.empty())
        {
            id = free_ids.front();
            free_ids.pop();
        }
        else
        {
            if (next_id >= max_entities)
            {
                throw std::runtime_error("Entity limit reached");
            }
            id = next_id.fetch_add(1, std::memory_order_relaxed);
        }

        EntityRecord& record = entity_records[id];
        assert(!record.alive && "Entity already alive");
        record.alive = true;

        return Entity{ id, record.generation };
    }

    void Destroy(Entity entity)
    {
        if (entity.id >= max_entities)
        {
            return;
        }

        // 세대가 다르면 이미 파괴된 엔티티이므로 무시
        EntityRecord& record = entity_records[entity.id];
        if (record.generation != entity.generation)
        {
            return;
        }

        assert(record.alive && "Entity already destroyed");
        record.alive = false;
        ++record.generation; // 세대 변경

        // ID 재활용용 큐에 저장
        free_ids.push(entity.id);
    }

    [[nodiscard]] bool IsValid(Entity entity) const
    {
        if (entity.id >= max_entities)
        {
            return false;
        }
        const EntityRecord& record = entity_records[entity.id];
        return record.alive && (record.generation == entity.generation);
    }

    [[nodiscard]] uint32 GetMaxEntities() const { return max_entities; }

private:
    struct EntityRecord
    {
        uint32 generation = 0;
        bool alive = false;
    };

    std::vector<EntityRecord> entity_records;
    std::queue<uint32> free_ids;

    uint32 max_entities;
    std::atomic<uint32> next_id;
};
}
