#include "Core/Reflection/TypeRegistry.h"

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
    [[maybe_unused]] TypeRegistry& registry = TypeRegistry::Get();

    // 기본 산술 타입
    registry.RegisterPrimitive<bool>()   .Serialize(&MakeSerialize<bool>);
    registry.RegisterPrimitive<int8>()   .Serialize(&MakeSerialize<int8>);
    registry.RegisterPrimitive<uint8>()  .Serialize(&MakeSerialize<uint8>);
    registry.RegisterPrimitive<int16>()  .Serialize(&MakeSerialize<int16>);
    registry.RegisterPrimitive<uint16>() .Serialize(&MakeSerialize<uint16>);
    registry.RegisterPrimitive<int32>()  .Serialize(&MakeSerialize<int32>);
    registry.RegisterPrimitive<uint32>() .Serialize(&MakeSerialize<uint32>);
    registry.RegisterPrimitive<int64>()  .Serialize(&MakeSerialize<int64>);
    registry.RegisterPrimitive<uint64>() .Serialize(&MakeSerialize<uint64>);
    registry.RegisterPrimitive<float>()  .Serialize(&MakeSerialize<float>);
    registry.RegisterPrimitive<double>() .Serialize(&MakeSerialize<double>);

    // 엔진 타입
    registry.RegisterPrimitive<String>()     .Serialize(&MakeSerialize<String>);
    registry.RegisterPrimitive<StringName>() .Serialize(&MakeSerialize<StringName>);
    registry.RegisterPrimitive<Guid>()       .Serialize(&MakeSerialize<Guid>);
    registry.RegisterPrimitive<TypeId>()     .Serialize(&MakeSerialize<TypeId>);

    return true;
}();
} // namespace
