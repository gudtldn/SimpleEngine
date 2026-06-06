#pragma once

#include "SimpleEngine/Core/Reflection/Traits.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>


namespace se
{
namespace detail
{
/**
 * TypeId의 상속 체인을 순회하며, derived_id가 base_id와 같거나 파생 관계인지 확인합니다.
 * @param derived_id 검사 대상 (파생 클래스) TypeId
 * @param base_id 기준이 되는 (부모 클래스) TypeId
 * @return derived_id가 base_id이거나, base_id의 파생 클래스이면 true
 */
[[nodiscard]] SE_CORE_API bool IsTypeDerivedFrom(const TypeId& derived_id, const TypeId& base_id);

/**
 * obj의 완전한 객체 포인터(void*)에서 bases 그래프를 재귀 탐색하여 to 타입 포인터로 업캐스트합니다.
 * @param instance from 타입의 완전한 객체 포인터
 * @param from instance의 TypeId
 * @param to 변환할 TypeId
 * @return 성공 시 to 타입의 포인터, 실패 시 nullptr
 */
[[nodiscard]] SE_CORE_API void* TryUpcast(void* instance, const TypeId& from, const TypeId& to);
} // namespace detail

/**
 * 객체가 지정한 타입 T이거나 T의 파생 클래스인지 검사합니다.
 * @tparam T 검사할 대상 타입
 * @param instance 검사할 객체의 포인터 (nullptr일 경우 false)
 * @return obj의 런타임 타입이 T이거나 T로부터 파생되었으면 true
 */
template <typename T, IntrusiveReflectable From>
[[nodiscard]] bool IsA(const From* instance)
{
    if (!instance)
    {
        return false;
    }
    return detail::IsTypeDerivedFrom(instance->GetTypeId(), TypeId::Of<T>());
}

template <typename T, IntrusiveReflectable From>
[[nodiscard]] bool IsA(const From& instance)
{
    return detail::IsTypeDerivedFrom(instance.GetTypeId(), TypeId::Of<T>());
}

/**
 * 두 타입의 상속 관계를 확인합니다.
 * @tparam Derived 자식 타입
 * @tparam Base 부모 타입
 * @return Derived가 Base이거나 Base로부터 파생되었으면 true
 */
template <IntrusiveReflectable Derived, IntrusiveReflectable Base>
[[nodiscard]] bool IsChildOf()
{
    return detail::IsTypeDerivedFrom(TypeId::Of<Derived>(), TypeId::Of<Base>());
}

/**
 * 주어진 TypeId가 Base 타입의 자식(또는 동일한 타입)인지 검사합니다.
 * @tparam Base 부모 타입
 * @param derived_id 검사할 TypeId
 * @return derived_id가 Base이거나 Base로부터 파생되었으면 true
 */
template <typename Base>
[[nodiscard]] bool IsChildOf(TypeId derived_id)
{
    return detail::IsTypeDerivedFrom(derived_id, TypeId::Of<Base>());
}

/**
 * 리플렉션을 통해 생성된 원시 포인터(void*)를 특정 타입(클래스 또는 인터페이스)으로 안전하게 캐스팅합니다.
 * @tparam To 대상 타입 (클래스 또는 인터페이스)
 * @param raw_instance 원본 객체의 포인터 (완전한 객체)
 * @param actual_type_id 원본 객체의 실제 런타임 TypeId
 * @return 성공 시 To* 포인터, 실패 시 nullptr
 */
template <typename To>
[[nodiscard]] To* CastFromRaw(void* raw_instance, TypeId actual_type_id)
{
    if (!raw_instance)
    {
        return nullptr;
    }

    // bases 그래프를 통해 업캐스트를 시도
    return static_cast<To*>(detail::TryUpcast(raw_instance, actual_type_id, TypeId::Of<To>()));
}

/**
 * 안전한 다운캐스팅을 수행합니다.
 * 상속 체인을 검사한 뒤, 유효하지 않으면 nullptr을 반환합니다.
 *
 * @tparam To 캐스팅할 대상 타입
 * @param instance 캐스팅할 객체의 포인터
 * @return 캐스팅에 성공하면 To* 포인터, 실패하면 nullptr
 */
template <typename To, IntrusiveReflectable From>
[[nodiscard]] To* Cast(From* instance)
{
    if (!instance)
    {
        return nullptr;
    }

    // From이 To를 상속 받았는지? (컴파일 타임 up-cast)
    if constexpr (std::derived_from<From, To>)
    {
        return static_cast<To*>(instance);
    }
    else
    {
        // GetCompleteObject()로 most-derived 포인터를 얻은 뒤 bases 그래프를 탐색
        void* complete = instance->GetCompleteObject();
        return static_cast<To*>(detail::TryUpcast(complete, instance->GetTypeId(), TypeId::Of<To>()));
    }
}

template <typename To, IntrusiveReflectable From>
[[nodiscard]] const To* Cast(const From* instance)
{
    if (!instance)
    {
        return nullptr;
    }

    // From이 To를 상속 받았는지? (컴파일 타임 up-cast)
    if constexpr (std::derived_from<From, To>)
    {
        return static_cast<const To*>(instance);
    }
    else
    {
        // GetCompleteObject()로 most-derived 포인터를 얻은 뒤 bases 그래프를 탐색
        void* complete = const_cast<From*>(instance)->GetCompleteObject();
        return static_cast<const To*>(detail::TryUpcast(complete, instance->GetTypeId(), TypeId::Of<To>()));
    }
}

/**
 * 다운캐스팅을 수행합니다.
 * 캐스팅에 실패하거나 nullptr이 전달되면 Assert가 발생합니다.
 * 릴리스 빌드에서는 검사 없이 static_cast를 수행합니다.
 *
 * @tparam To 캐스팅할 대상 타입
 * @param instance 캐스팅할 객체의 포인터 (nullptr 불가)
 * @return 캐스팅된 To* 포인터
 */
template <typename To, IntrusiveReflectable From>
[[nodiscard]] To* CastChecked(From* instance)
{
    SE_ASSERT(instance != nullptr, "CastChecked failed: Source pointer is null!");

    // GetCompleteObject()로 most-derived 포인터를 얻은 뒤 bases 그래프를 탐색
    void* complete = instance->GetCompleteObject();
    void* result = detail::TryUpcast(complete, instance->GetTypeId(), TypeId::Of<To>());
    SE_ASSERT(
        result != nullptr,
        "CastChecked failed: Cannot cast '{}' to '{}'!",
        instance->GetTypeId().GetName(), TypeId::Of<To>().GetName()
    );
    return static_cast<To*>(result);
}

template <typename To, IntrusiveReflectable From>
[[nodiscard]] const To* CastChecked(const From* instance)
{
    SE_ASSERT(instance != nullptr, "CastChecked failed: Source pointer is null!");

    // GetCompleteObject()로 most-derived 포인터를 얻은 뒤 bases 그래프를 탐색
    void* complete = const_cast<From*>(instance)->GetCompleteObject();
    void* result = detail::TryUpcast(complete, instance->GetTypeId(), TypeId::Of<To>());
    SE_ASSERT(
        result != nullptr,
        "CastChecked failed: Cannot cast '{}' to '{}'!",
        instance->GetTypeId().GetName(), TypeId::Of<To>().GetName()
    );
    return static_cast<const To*>(result);
}

/**
 * 런타임 타입이 정확히 To와 일치하는 경우에만 캐스팅합니다.
 * 상속 체인을 순회하지 않으므로, 파생 클래스는 매칭되지 않습니다.
 *
 * @tparam To 캐스팅할 대상 타입 (정확히 이 타입이어야 함)
 * @param instance 캐스팅할 객체의 포인터
 * @return 런타임 타입이 To와 동일하면 To* 포인터, 아니면 nullptr
 */
template <typename To, IntrusiveReflectable From>
[[nodiscard]] To* ExactCast(From* instance)
{
    if (instance && instance->GetTypeId() == TypeId::Of<To>())
    {
        // Multiple inheritance offset 보정을 위해 GetCompleteObject()를 사용
        return static_cast<To*>(instance->GetCompleteObject());
    }
    return nullptr;
}

template <typename To, IntrusiveReflectable From>
[[nodiscard]] const To* ExactCast(const From* instance)
{
    if (instance && instance->GetTypeId() == TypeId::Of<To>())
    {
        // Multiple inheritance offset 보정을 위해 GetCompleteObject()를 사용
        return static_cast<const To*>(const_cast<From*>(instance)->GetCompleteObject());
    }
    return nullptr;
}
} // namespace se
