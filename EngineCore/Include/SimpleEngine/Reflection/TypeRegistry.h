#pragma once
#include <unordered_map>

#include "Core/Types/StringName.h"
#include "SimpleEngine/Core/Containers/Optional.h"
#include "SimpleEngine/Reflection/Meta.h"


namespace se::reflection
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

    [[nodiscard]] Optional<const TypeInfo&> Find(const TypeId& type_id) const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const StringName& type_name) const;
    [[nodiscard]] const auto& GetAllTypes() const { return type_map; }

private:
    std::unordered_map<StringName, TypeId> name_map;
    std::unordered_map<TypeId, TypeInfo> type_map;
};
}
