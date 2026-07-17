#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"


namespace se
{
TypeId TypeId::FromHash(u64 in_hash)
{
    const TypeRegistry& registry = TypeRegistry::Get();
    if (const auto type_info = registry.Find(TypeId{ in_hash }))
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

StringView TypeId::GetName() const
{
    return TypeRegistry::Get().FindChecked(*this).name;
}
} // namespace se
