#pragma once
#include <unordered_map>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Reflection/Meta.h"


namespace se::refl
{
class SE_CORE_API TypeRegistry
{
    TypeRegistry() = default;

public:
    ~TypeRegistry() = default;

    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    TypeRegistry(TypeRegistry&&) = delete;
    TypeRegistry& operator=(TypeRegistry&&) = delete;

    static TypeRegistry& GetInstance();

public:
    void RegisterType(TypeInfo&& type_info);

    template <typename T>
    [[nodiscard]] Optional<const TypeInfo&> Find() const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const TypeId& type_id) const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const StringName& type_name) const;
    [[nodiscard]] const auto& GetAllTypes() const { return type_map; }

private:
    HashMap<StringName, TypeId> name_map;
    HashMap<TypeId, TypeInfo> type_map;
};

template <typename T>
Optional<const TypeInfo&> TypeRegistry::Find() const
{
    return Find(TypeId::Get<T>());
}
}
