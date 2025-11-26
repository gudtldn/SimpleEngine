#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Reflection/TypeRegistry.h"


namespace se::refl
{
TypeId TypeId::FromHash(uint64 in_hash)
{
    const TypeRegistry& registry = TypeRegistry::GetInstance();
    const TypeId temp_id = TypeId{ "UnknownType", in_hash };

    if (const Optional type_info_opt = registry.Find(temp_id))
    {
        return type_info_opt->type_id;
    }
    return TypeId{};
}

TypeId TypeId::FromName(const StringName& in_type_name)
{
    const TypeRegistry& registry = TypeRegistry::GetInstance();
    if (const Optional type_info_opt = registry.Find(in_type_name))
    {
        return type_info_opt->type_id;
    }
    return TypeId{};
}
}
