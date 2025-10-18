#pragma once
#include <unordered_map>

#include "SimpleEngine/Core/Containers/Optional.h"
#include "SimpleEngine/Reflection/Meta.h"


namespace se::reflection
{
class SE_CORE_API ReflectionRegistry
{
    ReflectionRegistry() = default;

public:
    ~ReflectionRegistry() = default;

    ReflectionRegistry(const ReflectionRegistry&) = delete;
    ReflectionRegistry& operator=(const ReflectionRegistry&) = delete;
    ReflectionRegistry(ReflectionRegistry&&) = delete;
    ReflectionRegistry& operator=(ReflectionRegistry&&) = delete;

    static ReflectionRegistry& GetInstance();

public:
    void RegisterType(const TypeInfo* type_info);

    const TypeInfo* Find(const TypeId& type_id) const;
    const std::unordered_map<TypeId, const TypeInfo*>& GetAllTypes() const;

private:
    std::unordered_map<TypeId, const TypeInfo*> type_map;
};
}
