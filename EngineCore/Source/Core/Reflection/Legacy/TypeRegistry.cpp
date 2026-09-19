#include "../../../../Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se
{
TypeRegistry_v1& TypeRegistry_v1::Get()
{
    static TypeRegistry_v1 instance;
    return instance;
}

void TypeRegistry_v1::Resolve()
{
    if (is_resolved)
    {
        return;
    }

    direct_derived_map.Clear();

    for (const TypeInfo_v1& info : type_map | std::views::values)
    {
        // info의 모든 base(인터페이스 포함)를 역방향으로 인덱싱
        for (const BaseInfo_v1& base : info.bases)
        {
            direct_derived_map.Entry(base.base_id).OrDefault().Push(&info);
        }
    }

    is_resolved = true;
}

Optional<const TypeInfo_v1&> TypeRegistry_v1::Find(const TypeId_v1& type_id) const
{
    return type_map.Find(type_id);
}

Optional<const TypeInfo_v1&> TypeRegistry_v1::Find(const StringName& type_name) const
{
    return name_map.Find(type_name).AndThen([this](const TypeId_v1& type_id)
    {
        return Find(type_id);
    });
}

const TypeInfo_v1& TypeRegistry_v1::FindChecked(const TypeId_v1& type_id) const
{
    SE_ASSERT(type_map.Contains(type_id), "The type is not registered yet! Make sure SE_END_REFLECT is called.");
    return type_map.FindChecked(type_id);
}

ArrayView<const TypeInfo_v1* const> TypeRegistry_v1::GetDerivedTypes(const TypeId_v1& base_id) const
{
    SE_ASSERT(is_resolved, "TypeRegistry::Resolve() must be called before querying derived types!");
    return direct_derived_map.Find(base_id)
        .As<ArrayView<const TypeInfo_v1* const>>()
        .ValueOrDefault();
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
    TypeRegistry_v1& registry = TypeRegistry_v1::Get();

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
    registry.RegisterPrimitive<TypeId_v1>()     .Serialize(&MakeSerialize<TypeId_v1>);

    return true;
}();
} // namespace
