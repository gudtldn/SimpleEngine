#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"


namespace se
{
TypeId TypeId::FromHash(uint64 in_hash)
{
    const TypeRegistry& registry = TypeRegistry::Get();
    const TypeId temp_id = TypeId{ "UnknownType", in_hash };

    if (const auto type_info = registry.Find(temp_id))
    {
        return type_info->type_id;
    }
    return TypeId{};
}

TypeId TypeId::FromName(const StringName& in_type_name)
{
    const TypeRegistry& registry = TypeRegistry::Get();
    if (const auto type_info = registry.Find(in_type_name))
    {
        return type_info->type_id;
    }
    return TypeId{};
}
} // namespace se
