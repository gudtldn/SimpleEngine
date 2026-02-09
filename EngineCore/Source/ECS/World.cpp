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

IStorage* World::GetStorage(const TypeId& type_id)
{
    if (const Optional storage_opt = component_storages.Find(type_id))
    {
        return storage_opt->get();
    }

    if (const Optional interface_opt = ComponentRegistry::Get().GetInterface(type_id))
    {
        return interface_opt->ensure_storage(*this);
    }
    return nullptr;
}
}  // namespace se::ecs
