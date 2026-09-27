#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"

#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Utility/Debug.h"

#include <mutex>


namespace se
{
SerializeOpsRegistry& SerializeOpsRegistry::Get()
{
    static SerializeOpsRegistry instance;
    return instance;
}

void SerializeOpsRegistry::Install(TypeId id, const SerializeOps& ops)
{
    std::scoped_lock lock{ RegistrationMutex() };
    SE_ASSERT_RELEASE(!ops_map.Contains(id), "SerializeOpsRegistry::Install: SerializeOps for type id {} is already installed.", id.Value());
    ops_map.Emplace(id, ops);
}

Optional<const SerializeOps&> SerializeOpsRegistry::Find(TypeId id) const
{
    std::scoped_lock lock{ RegistrationMutex() };
    return ops_map.Find(id);
}
} // namespace se
