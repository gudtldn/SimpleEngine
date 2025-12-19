#include "ECS/World.h"

#include <ranges>
#include "ECS/SparseSet.h"
#include "ECS/ComponentRegistry.h"


namespace se::ecs
{
void World::DestroyEntity(Entity entity)
{
    for (const auto& storage : component_storages | std::views::values)
    {
        storage->Remove(entity);
    }
    entity_manager.Destroy(entity);
}

Array<Entity> World::GetAliveEntities() const
{
    return entity_manager.GetAliveEntities();
}

IStorage* World::GetStorage(const refl::TypeId& type_id)
{
    if (const Optional storage_opt = component_storages.Find(type_id))
    {
        return storage_opt->get();
    }

    if (ComponentRegistry::Get().EnsureStorage(*this, type_id))
    {
        std::unique_ptr<IStorage> null_ptr;
        return component_storages.Find(type_id).ValueOr(null_ptr).get();
    }
    return nullptr;
}
}
