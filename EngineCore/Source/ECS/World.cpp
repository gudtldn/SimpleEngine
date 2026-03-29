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
    entity_manager.Destroy(entity);
}

Array<Entity> World::GetAliveEntities() const
{
    return entity_manager.GetAliveEntities();
}

IStorage* World::GetStorage(const TypeId& type_id)
{
    if (const Optional storage = component_storages.Find(type_id))
    {
        return storage->get();
    }

    if (const Optional ops = ComponentRegistry::Get().GetOps(type_id))
    {
        return ops->ensure_storage(*this);
    }
    return nullptr;
}
} // namespace se
