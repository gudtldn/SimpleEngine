#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Core/Reflection/TypeBuilder.h"


namespace se
{
/**
 * 런타임 타입 정보(RTTI)를 중앙에서 관리하는 전역 저장소
 * 컴파일 타임에 수집된 모든 리플렉션 데이터를 런타임에 검색(Look-up)할 수 있게 해줍니다.
 */
class SE_CORE_API TypeRegistry
{
    TypeRegistry() = default;

public:
    ~TypeRegistry() = default;

    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    TypeRegistry(TypeRegistry&&) = delete;
    TypeRegistry& operator=(TypeRegistry&&) = delete;

    static TypeRegistry& Get();

public:
    /**
     * Registry에 타입을 등록합니다.
     * @tparam T 등록할 타입
     */
    template <typename T>
    detail::TypeBuilder<T> Register();

    /**
     * Registry에 기본 타입(Primitive)을 등록합니다.
     * @tparam T 기본 타입
     */
    template <typename T>
    detail::TypeBuilder<T> RegisterPrimitive();

public:
    template <typename T>
    [[nodiscard]] Optional<const TypeInfo&> Find() const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const TypeId& type_id) const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const StringName& type_name) const;

    template <typename T>
    [[nodiscard]] const TypeInfo& FindChecked() const;
    [[nodiscard]] const TypeInfo& FindChecked(const TypeId& type_id) const;

    [[nodiscard]] const HashMap<TypeId, TypeInfo>& GetAllTypes() const { return type_map; }

private:
    HashMap<StringName, TypeId> name_map;
    HashMap<TypeId, TypeInfo> type_map;
};

template <typename T>
Optional<const TypeInfo&> TypeRegistry::Find() const
{
    return Find(TypeId::Get<T>());
}

template <typename T>
const TypeInfo& TypeRegistry::FindChecked() const
{
    const TypeId id = TypeId::Get<T>();
    SE_ASSERT(type_map.Contains(id), "Type '{}' is not registered yet! Make sure SE_END_REFLECT is called.", id.GetName());
    return type_map.FindChecked(id);
}

template <typename T>
detail::TypeBuilder<T> TypeRegistry::Register()
{
    const TypeId id = TypeId::Get<T>();

    SE_ASSERT(!type_map.Contains(id), "Type '{}' is already registered! Check your initialization logic.", id.GetName());
    TypeInfo& info = type_map.Emplace(id);

    // 기본 정보 채우기
    info.type_id = id;
    info.name = id.GetName();
    info.size = sizeof(T);
    info.alignment = alignof(T);
    info.kind = ETypeKind::Struct;

    SE_ASSERT(!name_map.Contains(info.name), "Type name '{}' collision detected!", info.name);
    name_map.Insert(info.name, id);

    return detail::TypeBuilder<T>(&info);
}

template <typename T>
detail::TypeBuilder<T> TypeRegistry::RegisterPrimitive()
{
    auto builder = Register<T>();
    builder.SetKind(ETypeKind::Primitive);
    return builder;
}
} // namespace se
