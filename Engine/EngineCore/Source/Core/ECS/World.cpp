module SimpleEngine.Core;
import :ECS.World;


namespace se::core::ecs
{
World::EntityChain World::CreateEntity()
{
    return { this, entity_manager.Create() };
}

void World::DestroyEntity(Entity entity)
{
    for (const auto& storage : component_storages | std::views::values)
    {
        storage->Remove(entity);
    }
    entity_manager.Destroy(entity);
}
}
