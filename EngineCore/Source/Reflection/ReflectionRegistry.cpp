#if false
#include "Reflection/ReflectionRegistry.h"


namespace se::reflection
{
ReflectionRegistry& ReflectionRegistry::GetInstance()
{
    static ReflectionRegistry instance;
    return instance;
}

void ReflectionRegistry::RegisterType(const TypeInfo* type_info)
{
    if (type_info)
    {
        type_map[type_info->type_id] = type_info;
    }
}

const TypeInfo* ReflectionRegistry::Find(const TypeId& type_id) const
{
    if (const auto it = type_map.find(type_id); it != type_map.end())
    {
        return it->second;
    }
    return nullptr;
}

const std::unordered_map<TypeId, const TypeInfo*>& ReflectionRegistry::GetAllTypes() const
{
    return type_map;
}
}
#endif
