#include "SimpleEngine/ECS/ECSRegistry.h"


namespace se
{
ECSRegistry& ECSRegistry::Get()
{
    static ECSRegistry instance;
    return instance;
}

Optional<const ComponentOps&> ECSRegistry::GetOps(const TypeId& type_id) const
{
    return operators.Find(type_id);
}
} // namespace se
