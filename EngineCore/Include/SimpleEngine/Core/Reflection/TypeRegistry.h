#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/Traits.h"
#include "SimpleEngine/Core/Reflection/TypeBuilder.h"
#include "SimpleEngine/Core/Types/StringName.h"

#include <ranges>


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
    /** 등록된 모든 리플렉션 데이터를 순회하여 인터페이스 캐시 등을 구축합니다. */
    void Resolve();

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

    /**
     * Registry에 열거형(Enum)을 등록합니다.
     * base_or_inner_id에 underlying type의 TypeId를 자동 설정합니다.
     * @tparam T 열거형 타입
     */
    template <typename T>
        requires traits::EnumType<T>
    detail::TypeBuilder<T> RegisterEnum();

public:
    template <typename T>
    [[nodiscard]] Optional<const TypeInfo&> Find() const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const TypeId& type_id) const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const StringName& type_name) const;

    template <typename T>
    [[nodiscard]] const TypeInfo& FindChecked() const;
    [[nodiscard]] const TypeInfo& FindChecked(const TypeId& type_id) const;

    [[nodiscard]] const HashMap<TypeId, TypeInfo>& GetAllTypes() const { return type_map; }

    /**
     * 특정 인터페이스를 구현(Implements)하는 모든 등록된 타입의 TypeInfo 목록을 반환합니다.
     * @tparam T 인터페이스 타입
     * @return 해당 인터페이스를 구현하는 TypeInfo 포인터 목록
     */
    template <typename T>
    [[nodiscard]] Array<const TypeInfo*> GetImplementations() const;

    [[nodiscard]] Array<const TypeInfo*> GetImplementations(const TypeId& interface_id) const;

private:
    HashMap<StringName, TypeId> name_map;
    HashMap<TypeId, TypeInfo> type_map;

    bool is_resolved = false;
    HashMap<TypeId, Array<const TypeInfo*>> interface_implementations_map;
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
    static_assert(
        Reflectable<T>,
        "Type T is not reflectable. Please use SE_CLASS() macro (intrusive) or SE_DECLARE_REFLECTION() macro (non-intrusive)."
    );

    const TypeId id = TypeId::Get<T>();

    SE_ASSERT(!type_map.Contains(id), "Type '{}' is already registered! Check your initialization logic.", id.GetName());
    TypeInfo& info = type_map.Emplace(id);

    // 기본 정보 채우기
    info.type_id = id;
    info.name = id.GetName();
    info.size = sizeof(T);
    info.alignment = alignof(T);

    SE_ASSERT(!name_map.Contains(info.name), "Type name '{}' collision detected!", info.name);
    name_map.Insert(info.name, id);

    is_resolved = false; // Resolve 캐시 무효화
    return detail::TypeBuilder<T>(&info, ETypeKind::Struct);
}

template <typename T>
detail::TypeBuilder<T> TypeRegistry::RegisterPrimitive()
{
    const TypeId id = TypeId::Get<T>();

    SE_ASSERT(!type_map.Contains(id), "Type '{}' is already registered! Check your initialization logic.", id.GetName());
    TypeInfo& info = type_map.Emplace(id);

    // 기본 정보 채우기
    info.type_id = id;
    info.name = id.GetName();
    info.size = sizeof(T);
    info.alignment = alignof(T);

    SE_ASSERT(!name_map.Contains(info.name), "Type name '{}' collision detected!", info.name);
    name_map.Insert(info.name, id);

    is_resolved = false; // Resolve 캐시 무효화
    return detail::TypeBuilder<T>(&info, ETypeKind::Primitive);
}

template <typename T>
    requires traits::EnumType<T>
detail::TypeBuilder<T> TypeRegistry::RegisterEnum()
{
    const TypeId id = TypeId::Get<T>();

    SE_ASSERT(!type_map.Contains(id), "Type '{}' is already registered! Check your initialization logic.", id.GetName());
    TypeInfo& info = type_map.Emplace(id);

    // 기본 정보 채우기
    info.type_id = id;
    info.name = id.GetName();
    info.size = sizeof(T);
    info.alignment = alignof(T);
    info.base_or_inner_id = TypeId::Get<std::underlying_type_t<T>>();

    SE_ASSERT(!name_map.Contains(info.name), "Type name '{}' collision detected!", info.name);
    name_map.Insert(info.name, id);

    is_resolved = false; // Resolve 캐시 무효화
    return detail::TypeBuilder<T>(&info, ETypeKind::Enum);
}

template <typename T>
Array<const TypeInfo*> TypeRegistry::GetImplementations() const
{
    const TypeId interface_id = TypeId::Get<T>();
    return GetImplementations(interface_id);
}
} // namespace se
