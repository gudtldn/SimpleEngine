#include "ECS/ComponentRegistry.h"


namespace se::ecs
{
ComponentRegistry& ComponentRegistry::Get()
{
    static ComponentRegistry instance;
    return instance;
}

Optional<const ComponentInterface&> ComponentRegistry::GetInterface(const TypeId& type_id) const
{
    return interfaces.Find(type_id);
}
}  // namespace se::ecs
