module SE.Core;
import :ECS.World;


namespace se::core::ecs
{
void World::DestroyEntity(Entity entity)
{
    for (const auto& storage : component_storages | std::views::values)
    {
        storage->Remove(entity);
    }
    entity_manager.Destroy(entity);
}

vector<Entity> World::GetAliveEntities() const
{
    return entity_manager.GetAliveEntities();
}
}
