#include "../../../../Include/SimpleEngine/Core/Reflection/Legacy/TypeId.h"
#include "SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"


namespace se
{
TypeId_v1 TypeId_v1::FromHash(u64 in_hash)
{
    const TypeRegistry_v1& registry = TypeRegistry_v1::Get();
    if (const auto type_info = registry.Find(TypeId_v1{ in_hash }))
    {
        return type_info->type_id;
    }
    return TypeId_v1{};
}

TypeId_v1 TypeId_v1::FromName(const StringName& in_type_name)
{
    const TypeRegistry_v1& registry = TypeRegistry_v1::Get();
    if (const auto type_info = registry.Find(in_type_name))
    {
        return type_info->type_id;
    }
    return TypeId_v1{};
}

StringView TypeId_v1::GetName() const
{
    return TypeRegistry_v1::Get().FindChecked(*this).name;
}
} // namespace se
