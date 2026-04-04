#include "SimpleEngine/ECS/ECSRegistry.h"


namespace se
{
ECSRegistry& ECSRegistry::Get()
{
    static ECSRegistry instance;
    return instance;
}

Optional<const ComponentOps&> ECSRegistry::GetComponentOps(const TypeId& type_id) const
{
    return component_operators.Find(type_id);
}

Optional<const ResourceOps&> ECSRegistry::GetResourceOps(const TypeId& type_id) const
{
    return resource_operators.Find(type_id);
}
} // namespace se
