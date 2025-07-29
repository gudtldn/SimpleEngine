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
        : entity_records(in_max_entities), max_entities(in_max_entities)
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
            id = static_cast<uint32>(entity_records.size());
            if (id >= max_entities)
            {
                throw std::runtime_error("Entity limit reached");
            }
            entity_records.push_back({});
        }

        auto& record = entity_records[id];
        assert(!record.alive && "Entity already alive");
        record.alive = true;

        return Entity{ id, record.generation };
    }

    void Destroy(Entity entity)
    {
        if (entity.id >= entity_records.size())
        {
            return;
        }

        auto& record = entity_records[entity.id];
        // 세대가 다르면 이미 파괴된 엔티티이므로 무시
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
        if (entity.id >= entity_records.size())
        {
            return false;
        }
        const auto& record = entity_records[entity.id];
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
};
}
