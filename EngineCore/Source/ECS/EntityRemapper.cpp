#include "SimpleEngine/ECS/EntityRemapper.h"


namespace se
{
bool EntityRemapper::Add(Entity entity, u64 persistent_id)
{
    // 0은 null Entity의 영속 ID
    if (!entity.IsValid() || persistent_id == 0)
    {
        return false;
    }
    if (id_by_entity.Contains(entity) || entity_by_id.Contains(persistent_id))
    {
        return false;
    }

    id_by_entity.Insert(entity, persistent_id);
    entity_by_id.Insert(persistent_id, entity);
    return true;
}

Optional<u64> EntityRemapper::ToPersistentId(Entity entity) const
{
    if (!entity.IsValid())
    {
        return u64{ 0 };
    }
    if (const auto persistent_id = id_by_entity.Find(entity))
    {
        return *persistent_id;
    }
    return NullOpt;
}

Entity EntityRemapper::ToEntity(u64 persistent_id)
{
    if (persistent_id == 0)
    {
        return {};
    }
    if (const auto entity = entity_by_id.Find(persistent_id))
    {
        return *entity;
    }

    // 없는 엔티티를 가리키는 참조는 끊고, 로더가 알릴 수 있게 기록
    if (!unresolved_ids.Contains(persistent_id))
    {
        unresolved_ids.Push(persistent_id);
    }
    return {};
}

ArrayView<const u64> EntityRemapper::GetUnresolvedIds() const
{
    return unresolved_ids;
}
} // namespace se
