#include "Reflection/TypeRegistry.h"


namespace se::refl
{
TypeRegistry& TypeRegistry::Get()
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
    return type_map.Find(type_id);
}

Optional<const TypeInfo&> TypeRegistry::Find(const StringName& type_name) const
{
    return name_map.Find(type_name).AndThen([this](const TypeId& type_id)
    {
        return Find(type_id);
    });
}
}
