#include "ECS/EntityManager.h"

#include <cassert>


namespace se::world
{
Entity EntityManager::Create()
{
    uint32 id;

    // 재활용 가능한 ID 있으면 우선 사용
    if (Optional<uint32> id_opt = free_ids.Pop())
    {
        id = *id_opt;
    }
    else
    {
        id = next_id.fetch_add(1, std::memory_order_relaxed);
        entity_records.Emplace();
    }

    EntityRecord& record = entity_records[id];
    assert(!record.alive && "Entity already alive");
    record.alive = true;

    return Entity{ id, record.generation };
}

void EntityManager::Destroy(Entity entity)
{
    if (entity.id >= entity_records.Len())
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
    free_ids.Push(entity.id);
}

Array<Entity> EntityManager::GetAliveEntities() const
{
    Array<Entity> alive_entities;
    alive_entities.Reserve(next_id);
    for (uint32 id = 0; id < next_id; ++id)
    {
        if (entity_records[id].alive)
        {
            alive_entities.Push({ id, entity_records[id].generation });
        }
    }
    return alive_entities;
}

bool EntityManager::IsValid(Entity entity) const
{
    if (entity.id >= entity_records.Len())
    {
        return false;
    }
    const EntityRecord& record = entity_records[entity.id];
    return record.alive && (record.generation == entity.generation);
}
}
