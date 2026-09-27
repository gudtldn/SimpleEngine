#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"

#include "SimpleEngine/Core/Reflection/TypeRegistry.h"

#include <mutex>


namespace se
{
ValueOpsRegistry& ValueOpsRegistry::Get()
{
    static ValueOpsRegistry instance;
    return instance;
}

void ValueOpsRegistry::Install(TypeId id, const ValueOps& ops)
{
    std::scoped_lock lock{ RegistrationMutex() };
    ops_map.Entry(id).OrDefault() = ops;
}

Optional<const ValueOps&> ValueOpsRegistry::Find(TypeId id) const
{
    std::scoped_lock lock{ RegistrationMutex() };
    return ops_map.Find(id);
}
} // namespace se
