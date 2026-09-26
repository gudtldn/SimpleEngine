#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"

#include "SimpleEngine/Utility/Debug.h"


namespace se
{
SerializeOpsRegistry& SerializeOpsRegistry::Get()
{
    static SerializeOpsRegistry instance;
    return instance;
}

void SerializeOpsRegistry::Install(TypeId id, const SerializeOps& ops)
{
    SE_ASSERT_RELEASE(!ops_map.Contains(id), "SerializeOpsRegistry::Install: SerializeOps for type id {} is already installed.", id.Value());
    ops_map.Emplace(id, ops);
}

Optional<const SerializeOps&> SerializeOpsRegistry::Find(TypeId id) const
{
    return ops_map.Find(id);
}
} // namespace se
