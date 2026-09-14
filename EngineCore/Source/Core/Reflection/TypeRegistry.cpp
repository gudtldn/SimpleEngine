#include "SimpleEngine/Core/Reflection/TypeRegistry.h"

#include "SimpleEngine/Utility/Debug.h"


namespace se
{
TypeRegistry& TypeRegistry::Get()
{
    static TypeRegistry instance;
    return instance;
}

TypeInfo& TypeRegistry::Emplace(TypeId id)
{
    TypeInfo& info = type_map.Entry(id).OrDefault();
    info.id = id;
    return info;
}

Array<FieldInfo>& TypeRegistry::EmplaceFieldStorage(TypeId id)
{
    return field_storage.Entry(id).OrDefault();
}

Array<EnumEntry>& TypeRegistry::EmplaceEnumEntryStorage(TypeId id)
{
    return enum_entry_storage.Entry(id).OrDefault();
}

Optional<const TypeInfo&> TypeRegistry::Find(TypeId id) const
{
    return type_map.Find(id);
}

const TypeInfo& TypeRegistry::FindChecked(TypeId id) const
{
    SE_ASSERT(type_map.Contains(id), "The type is not registered yet! Make sure EnsureRegistered<T>() was called.");
    return type_map.FindChecked(id);
}

Array<const TypeInfo*> TypeRegistry::GetAllTypes() const
{
    return Array<const TypeInfo*>::FromRange(type_map | std::views::values | std::views::transform([](const TypeInfo& info)
    {
        return &info;
    }));
}
} // namespace se
