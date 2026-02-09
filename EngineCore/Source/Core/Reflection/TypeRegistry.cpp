#include "Core/Reflection/TypeRegistry.h"


namespace se
{
TypeRegistry& TypeRegistry::Get()
{
    static TypeRegistry instance;
    return instance;
}

Optional<const TypeInfo&> TypeRegistry::Find(const TypeId& type_id) const
{
    return type_map.Find(type_id);
}

Optional<const TypeInfo&> TypeRegistry::Find(const StringName& type_name) const
{
    return name_map.Find(type_name).AndThen([this](const TypeId& type_id)
    {
        return Find(type_id);
    });
}

const TypeInfo& TypeRegistry::FindChecked(const TypeId& type_id) const
{
    SE_ASSERT(type_map.Contains(type_id), "Type '{}' is not registered yet! Make sure SE_END_REFLECT is called.", type_id.GetName());
    return type_map.FindChecked(type_id);
}
}  // namespace se
