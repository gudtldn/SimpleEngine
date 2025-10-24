#include "Reflection/TypeRegistry.h"


namespace se::refl
{
TypeRegistry& TypeRegistry::GetInstance()
{
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::RegisterType(TypeInfo&& type_info)
{
    name_map[type_info.name] = type_info.type_id;
    type_map[type_info.type_id] = std::move(type_info);
}

Optional<const TypeInfo&> TypeRegistry::Find(const TypeId& type_id) const
{
    if (const auto it = type_map.find(type_id); it != type_map.end())
    {
        return it->second;
    }
    return std::nullopt;
}

Optional<const TypeInfo&> TypeRegistry::Find(const StringName& type_name) const
{
    if (const auto it = name_map.find(type_name); it != name_map.end())
    {
        return Find(it->second);
    }
    return std::nullopt;
}
}
