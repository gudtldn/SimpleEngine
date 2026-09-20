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
    SE_ASSERT(!type_map.Contains(id), "TypeId collision! A different type is already registered under this id.");

    TypeInfo& info = type_map.Entry(id).OrDefault();
    info.id = id;
    return info;
}

StructStorage& TypeRegistry::EmplaceStructStorage(TypeId id)
{
    return struct_storage.Entry(id).OrDefault();
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
} // namespace se
