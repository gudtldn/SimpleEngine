#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
Entity EntityManager::Create()
{
    uint32 id;

    // 재활용 가능한 슬롯이 있으면 우선 사용 (intrusive free list)
    if (free_list_head != ENTITY_FREE_LIST_END)
    {
        id = free_list_head;
        free_list_head = entity_records[id].next_free;
    }
    else
    {
        id = next_id.fetch_add(1, std::memory_order_relaxed);
        entity_records.Emplace();
    }

    EntityRecord& record = entity_records[id];
    SE_ASSERT(!record.IsAlive(), "Entity already alive");
    record.next_free = ENTITY_ALIVE;

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

    SE_ASSERT(record.IsAlive(), "Entity already destroyed");
    ++record.generation; // 세대 변경

    // intrusive free list에 prepend
    record.next_free = std::exchange(free_list_head, entity.id);
}

bool EntityManager::IsValid(Entity entity) const
{
    if (entity.id >= entity_records.Len())
    {
        return false;
    }

    const EntityRecord& record = entity_records[entity.id];
    return record.IsAlive() && (record.generation == entity.generation);
}

Optional<Entity> EntityManager::TryResolveEntity(uint32 id) const
{
    if (id >= entity_records.Len())
    {
        return NullOpt;
    }

    const EntityRecord& record = entity_records[id];
    if (!record.IsAlive())
    {
        return NullOpt;
    }

    return Entity{ id, record.generation };
}
} // namespace se
