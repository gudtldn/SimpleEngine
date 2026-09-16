#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"


namespace se
{
ValueOpsRegistry& ValueOpsRegistry::Get()
{
    static ValueOpsRegistry instance;
    return instance;
}

void ValueOpsRegistry::Install(TypeId id, const ValueOps& ops)
{
    ops_map.Entry(id).OrDefault() = ops;
}

Optional<const ValueOps&> ValueOpsRegistry::Find(TypeId id) const
{
    return ops_map.Find(id);
}
} // namespace se
