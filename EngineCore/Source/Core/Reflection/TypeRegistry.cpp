#include "SimpleEngine/Core/Reflection/TypeRegistry.h"

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se
{
TypeRegistry& TypeRegistry::Get()
{
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::Resolve()
{
    if (is_resolved)
    {
        return;
    }

    interface_implementations_map.Clear();

    for (const TypeInfo& info : type_map | std::views::values)
    {
        // info가 구현하는 모든 인터페이스를 순회
        for (const TypeId& interface_id : info.interfaces | std::views::keys)
        {
            interface_implementations_map[interface_id].Push(&info);
        }
    }

    is_resolved = true;
}

Optional<const TypeInfo&> TypeRegistry::Find(const TypeId& type_id) const
{
    return type_map.Find(type_id);
}

Optional<const TypeInfo&> TypeRegistry::Find(const StringName& type_name) const
{
    return name_map.Find(type_name).AndThen([this](const TypeId& type_id)
    {
        return Find(type_id);
    });
}

const TypeInfo& TypeRegistry::FindChecked(const TypeId& type_id) const
{
    SE_ASSERT(type_map.Contains(type_id), "Type '{}' is not registered yet! Make sure SE_END_REFLECT is called.", type_id.GetName());
    return type_map.FindChecked(type_id);
}

Array<const TypeInfo*> TypeRegistry::GetImplementations(const TypeId& interface_id) const
{
    SE_ASSERT(is_resolved, "TypeRegistry::Resolve() must be called before querying implementations!");
    return interface_implementations_map.Find(interface_id).Copy().ValueOrDefault();
}
} // namespace se

namespace
{
using namespace se;

template <typename T>
void MakeSerialize(Archive& ar, void* ptr)
{
    ar << *static_cast<T*>(ptr);
}

[[maybe_unused]] const bool Primitive_Registrar = [] -> bool
{
    TypeRegistry& registry = TypeRegistry::Get();

    // 기본 산술 타입
    registry.RegisterPrimitive<bool>()   .Serialize(&MakeSerialize<bool>);
    registry.RegisterPrimitive<i8>()   .Serialize(&MakeSerialize<i8>);
    registry.RegisterPrimitive<u8>()  .Serialize(&MakeSerialize<u8>);
    registry.RegisterPrimitive<i16>()  .Serialize(&MakeSerialize<i16>);
    registry.RegisterPrimitive<u16>() .Serialize(&MakeSerialize<u16>);
    registry.RegisterPrimitive<i32>()  .Serialize(&MakeSerialize<i32>);
    registry.RegisterPrimitive<u32>() .Serialize(&MakeSerialize<u32>);
    registry.RegisterPrimitive<i64>()  .Serialize(&MakeSerialize<i64>);
    registry.RegisterPrimitive<u64>() .Serialize(&MakeSerialize<u64>);
    registry.RegisterPrimitive<f32>()  .Serialize(&MakeSerialize<f32>);
    registry.RegisterPrimitive<f64>() .Serialize(&MakeSerialize<f64>);

    // 엔진 타입
    registry.RegisterPrimitive<String>()     .Serialize(&MakeSerialize<String>);
    registry.RegisterPrimitive<StringName>() .Serialize(&MakeSerialize<StringName>);
    registry.RegisterPrimitive<Guid>()       .Serialize(&MakeSerialize<Guid>);
    registry.RegisterPrimitive<TypeId>()     .Serialize(&MakeSerialize<TypeId>);

    return true;
}();
} // namespace
