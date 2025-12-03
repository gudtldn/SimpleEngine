#include "ECS/World.h"

#include <ranges>
#include "ECS/SparseSet.h"


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
}
