#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "Traits.h"
#include "SimpleEngine/Core/Reflection/Legacy//TypeBuilder.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se
{
/**
 * 런타임 타입 정보(RTTI)를 중앙에서 관리하는 전역 저장소
 * 컴파일 타임에 수집된 모든 리플렉션 데이터를 런타임에 검색(Look-up)할 수 있게 해줍니다.
 */
class SE_CORE_API TypeRegistry_v1
{
    TypeRegistry_v1() = default;

public:
    ~TypeRegistry_v1() = default;

    TypeRegistry_v1(const TypeRegistry_v1&) = delete;
    TypeRegistry_v1& operator=(const TypeRegistry_v1&) = delete;
    TypeRegistry_v1(TypeRegistry_v1&&) = delete;
    TypeRegistry_v1& operator=(TypeRegistry_v1&&) = delete;

    static TypeRegistry_v1& Get();

public:
    /** 등록된 모든 리플렉션 데이터를 순회하여 인터페이스 캐시 등을 구축합니다. */
    void Resolve();

    /**
     * Registry에 타입을 등록합니다.
     * @tparam T 등록할 타입
     */
    template <typename T>
    detail::TypeBuilder_v1<T> Register();

    /**
     * Registry에 기본 타입(Primitive)을 등록합니다.
     * @tparam T 기본 타입
     */
    template <typename T>
    detail::TypeBuilder_v1<T> RegisterPrimitive();

    /**
     * Registry에 열거형(Enum)을 등록합니다.
     * base_or_inner_id에 underlying type의 TypeId를 자동 설정합니다.
     * @tparam T 열거형 타입
     */
    template <typename T>
        requires traits::EnumType<T>
    detail::TypeBuilder_v1<T> RegisterEnum();

public:
    template <typename T>
    [[nodiscard]] Optional<const TypeInfo_v1&> Find() const;
    [[nodiscard]] Optional<const TypeInfo_v1&> Find(const TypeId_v1& type_id) const;
    [[nodiscard]] Optional<const TypeInfo_v1&> Find(const StringName& type_name) const;

    template <typename T>
    [[nodiscard]] const TypeInfo_v1& FindChecked() const;
    [[nodiscard]] const TypeInfo_v1& FindChecked(const TypeId_v1& type_id) const;

    [[nodiscard]] const HashMap<TypeId_v1, TypeInfo_v1>& GetAllTypes() const { return type_map; }

    /**
     * 특정 타입을 직접 상속/구현하는 모든 등록된 타입의 TypeInfo 목록을 반환합니다.
     * @note 인터페이스 구현체 조회에도 사용합니다.
     * @tparam T 부모/인터페이스 타입
     * @return 해당 타입을 직접 base로 가지는 TypeInfo 포인터 뷰
     */
    template <typename T>
    [[nodiscard]] ArrayView<const TypeInfo_v1* const> GetDerivedTypes() const;

    [[nodiscard]] ArrayView<const TypeInfo_v1* const> GetDerivedTypes(const TypeId_v1& base_id) const;

private:
    HashMap<StringName, TypeId_v1> name_map;
    HashMap<TypeId_v1, TypeInfo_v1> type_map;

    bool is_resolved = false;
    HashMap<TypeId_v1, Array<const TypeInfo_v1*>> direct_derived_map;
};

template <typename T>
Optional<const TypeInfo_v1&> TypeRegistry_v1::Find() const
{
    return Find(TypeId_v1::Of<T>());
}

template <typename T>
const TypeInfo_v1& TypeRegistry_v1::FindChecked() const
{
    const TypeId_v1 id = TypeId_v1::Of<T>();
    const StringView name = GetFullTypeName<T>();

    SE_ASSERT(type_map.Contains(id), "Type '{}' is not registered yet! Make sure SE_END_REFLECT is called.", name);
    return type_map.FindChecked(id);
}

template <typename T>
detail::TypeBuilder_v1<T> TypeRegistry_v1::Register()
{
    static_assert(
        Reflectable_v1<T>,
        "Type T is not reflectable. Please use SE_CLASS_V1() macro (intrusive) or SE_DECLARE_REFLECTION_V1() macro (non-intrusive)."
    );

    const TypeId_v1 id = TypeId_v1::Of<T>();
    const StringView name = GetFullTypeName<T>();

    SE_ASSERT(!type_map.Contains(id), "Type '{}' is already registered! Check your initialization logic.", name);
    TypeInfo_v1& info = type_map.Emplace(id);

    // 기본 정보 채우기
    info.type_id = id;
    info.name = name;
    info.size = sizeof(T);
    info.alignment = alignof(T);

    SE_ASSERT(!name_map.Contains(info.name), "Type name '{}' collision detected!", info.name);
    name_map.Insert(info.name, id);

    is_resolved = false; // Resolve 캐시 무효화
    return detail::TypeBuilder_v1<T>(&info, ETypeKind_v1::Struct);
}

template <typename T>
detail::TypeBuilder_v1<T> TypeRegistry_v1::RegisterPrimitive()
{
    const TypeId_v1 id = TypeId_v1::Of<T>();
    const StringView name = GetFullTypeName<T>();

    SE_ASSERT(!type_map.Contains(id), "Type '{}' is already registered! Check your initialization logic.", name);
    TypeInfo_v1& info = type_map.Emplace(id);

    // 기본 정보 채우기
    info.type_id = id;
    info.name = name;
    info.size = sizeof(T);
    info.alignment = alignof(T);

    SE_ASSERT(!name_map.Contains(info.name), "Type name '{}' collision detected!", info.name);
    name_map.Insert(info.name, id);

    is_resolved = false; // Resolve 캐시 무효화
    return detail::TypeBuilder_v1<T>(&info, ETypeKind_v1::Primitive);
}

template <typename T>
    requires traits::EnumType<T>
detail::TypeBuilder_v1<T> TypeRegistry_v1::RegisterEnum()
{
    const TypeId_v1 id = TypeId_v1::Of<T>();
    const StringView name = GetFullTypeName<T>();

    SE_ASSERT(!type_map.Contains(id), "Type '{}' is already registered! Check your initialization logic.", name);
    TypeInfo_v1& info = type_map.Emplace(id);

    // 기본 정보 채우기
    info.type_id = id;
    info.name = name;
    info.size = sizeof(T);
    info.alignment = alignof(T);
    info.inner_type_id = TypeId_v1::Of<std::underlying_type_t<T>>();

    SE_ASSERT(!name_map.Contains(info.name), "Type name '{}' collision detected!", info.name);
    name_map.Insert(info.name, id);

    is_resolved = false; // Resolve 캐시 무효화
    return detail::TypeBuilder_v1<T>(&info, ETypeKind_v1::Enum);
}

template <typename T>
ArrayView<const TypeInfo_v1* const> TypeRegistry_v1::GetDerivedTypes() const
{
    const TypeId_v1 base_id = TypeId_v1::Of<T>();
    return GetDerivedTypes(base_id);
}
} // namespace se
