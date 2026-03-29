#include "SimpleEngine/ECS/ComponentRegistry.h"


namespace se
{
ComponentRegistry& ComponentRegistry::Get()
{
    static ComponentRegistry instance;
    return instance;
}

Optional<const ComponentOps&> ComponentRegistry::GetOps(const TypeId& type_id) const
{
    return operators.Find(type_id);
}
} // namespace se
