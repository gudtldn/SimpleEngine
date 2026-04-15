#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
Entity EntityManager::Create()
{
    uint32 id;

    // 재활용 가능한 ID가 있으면 우선 사용
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
    SE_ASSERT(!record.alive, "Entity already alive");
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

    SE_ASSERT(record.alive, "Entity already destroyed");
    record.alive = false;
    ++record.generation; // 세대 변경

    // ID 재활용용 큐에 저장
    free_ids.Push(entity.id);
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

Optional<Entity> EntityManager::TryResolveEntity(uint32 id) const
{
    if (id >= entity_records.Len())
    {
        return NullOpt;
    }
    const EntityRecord& record = entity_records[id];
    if (!record.alive)
    {
        return NullOpt;
    }
    return Entity{ id, record.generation };
}
} // namespace se
