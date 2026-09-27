#include "SimpleEngine/Core/Reflection/TypeRegistry.h"

#include "SimpleEngine/Utility/Debug.h"


namespace se
{
std::recursive_mutex& RegistrationMutex()
{
    static std::recursive_mutex mutex;
    return mutex;
}

TypeRegistry& TypeRegistry::Get()
{
    static TypeRegistry instance;
    return instance;
}

TypeInfo& TypeRegistry::Emplace(TypeId id)
{
    std::scoped_lock lock{ RegistrationMutex() };
    SE_ASSERT(!type_map.Contains(id), "TypeId collision! A different type is already registered under this id.");

    TypeInfo& info = type_map.Entry(id).OrDefault();
    info.id = id;
    return info;
}

StructStorage& TypeRegistry::EmplaceStructStorage(TypeId id)
{
    std::scoped_lock lock{ RegistrationMutex() };
    return struct_storage.Entry(id).OrDefault();
}

Array<EnumEntry>& TypeRegistry::EmplaceEnumEntryStorage(TypeId id)
{
    std::scoped_lock lock{ RegistrationMutex() };
    return enum_entry_storage.Entry(id).OrDefault();
}

Optional<const TypeInfo&> TypeRegistry::Find(TypeId id) const
{
    std::scoped_lock lock{ RegistrationMutex() };
    return type_map.Find(id);
}

const TypeInfo& TypeRegistry::FindChecked(TypeId id) const
{
    std::scoped_lock lock{ RegistrationMutex() };
    SE_ASSERT(type_map.Contains(id), "The type is not registered yet! Make sure EnsureRegistered<T>() was called.");
    return type_map.FindChecked(id);
}

Array<const TypeInfo*> TypeRegistry::GetAllTypes() const
{
    std::scoped_lock lock{ RegistrationMutex() };

    Array<const TypeInfo*> types;
    types.Reserve(type_map.Len());
    for (const auto& pair : type_map)
    {
        types.Push(&pair.second);
    }
    return types;
}
} // namespace se
