#include "SimpleEngine/ECS/World.h"

#include "SimpleEngine/ECS/ComponentStorage.h"
#include "SimpleEngine/ECS/ComponentRegistry.h"

#include <ranges>


namespace se
{
void World::DestroyEntity(Entity entity)
{
    for (const auto& storage : component_storages | std::views::values)
    {
        storage->Remove(entity);
    }

    // TODO: 여기 최적화의 여지가 있음. 지금 선형 탐색중, 추후 FlatSet이나 이진탐색으로 최적화 가능
    if (const Optional<usize> idx = alive_entities.Find(entity))
    {
        alive_entities.RemoveAtSwap(*idx);
    }

    entity_manager.Destroy(entity);
}

IStorage* World::GetOrCreateRawStorage(const TypeId& type_id)
{
    if (const auto storage = component_storages.Find(type_id))
    {
        return storage->get();
    }

    if (const auto ops = ComponentRegistry::Get().GetOps(type_id))
    {
        return ops->ensure_storage(*this);
    }

    return nullptr;
}

IStorage* World::FindRawStorage(const TypeId& type_id)
{
    if (const auto storage = component_storages.Find(type_id))
    {
        return storage->get();
    }

    return nullptr;
}

const IStorage* World::FindRawStorage(const TypeId& type_id) const
{
    if (const auto storage = component_storages.Find(type_id))
    {
        return storage->get();
    }

    return nullptr;
}
} // namespace se
