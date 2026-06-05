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
 * 주어진 TypeId가 특정 인터페이스를 구현하는지 확인합니다.
 * @param type_id 검사 대상 타입의 TypeId
 * @param interface_id 검사할 인터페이스의 TypeId
 * @return type_id가 interface_id를 구현하면 true
 */
[[nodiscard]] SE_CORE_API bool IsTypeImplementsInterface(const TypeId& type_id, const TypeId& interface_id);

/**
 * 주어진 TypeId를 특정 인터페이스 포인터로 캐스팅합니다.
 * @param instance 원본 객체의 포인터
 * @param type_id 원본 객체의 TypeId
 * @param interface_id 캐스팅할 인터페이스의 TypeId
 * @return 성공 시 인터페이스 포인터, 실패 시 nullptr
 */
[[nodiscard]] SE_CORE_API void* CastToInterface(void* instance, const TypeId& type_id, const TypeId& interface_id);
} // namespace detail

/**
 * 객체가 지정한 타입 T이거나 T의 파생 클래스인지 검사합니다.
 * @tparam T 검사할 대상 타입
 * @param obj 검사할 객체의 포인터 (nullptr일 경우 false)
 * @return obj의 런타임 타입이 T이거나 T로부터 파생되었으면 true
 */
template <typename T, IntrusiveReflectable From>
[[nodiscard]] bool IsA(const From* obj)
{
    if (!obj)
    {
        return false;
    }
    return detail::IsTypeDerivedFrom(obj->GetTypeId(), TypeId::Of<T>());
}

template <typename T, IntrusiveReflectable From>
[[nodiscard]] bool IsA(const From& obj)
{
    return detail::IsTypeDerivedFrom(obj.GetTypeId(), TypeId::Of<T>());
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
template <IntrusiveReflectable Base>
[[nodiscard]] bool IsChildOf(TypeId derived_id)
{
    return detail::IsTypeDerivedFrom(derived_id, TypeId::Of<Base>());
}

/**
 * 객체가 특정 인터페이스를 구현하는지 검사합니다.
 * @tparam Interface 검사할 인터페이스 타입
 * @param obj 검사할 객체의 포인터 (nullptr일 경우 false)
 * @return obj가 Interface를 구현하면 true
 */
template <typename Interface, IntrusiveReflectable From>
[[nodiscard]] bool Implements(const From* obj)
{
    if (!obj)
    {
        return false;
    }
    return detail::IsTypeImplementsInterface(obj->GetTypeId(), TypeId::Of<Interface>());
}

/**
 * 주어진 TypeId가 특정 인터페이스를 구현하는지 검사합니다.
 * @tparam Interface 검사할 인터페이스 타입
 * @param type_id 검사할 TypeId
 * @return type_id가 Interface를 구현하면 true
 */
template <typename Interface>
[[nodiscard]] bool Implements(TypeId type_id)
{
    return detail::IsTypeImplementsInterface(type_id, TypeId::Of<Interface>());
}

/**
 * 리플렉션을 통해 생성된 원시 포인터(void*)를 특정 타입(클래스 또는 인터페이스)으로 안전하게 캐스팅합니다.
 * @tparam To 대상 타입 (클래스 또는 인터페이스)
 * @param raw_instance 원본 객체의 포인터
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

    // 일반 상속 (Base Class) 캐스팅 시도
    if (detail::IsTypeDerivedFrom(actual_type_id, TypeId::Of<To>()))
    {
        return static_cast<To*>(raw_instance);
    }

    // 인터페이스 캐스팅 시도
    if (void* result = detail::CastToInterface(raw_instance, actual_type_id, TypeId::Of<To>()))
    {
        return static_cast<To*>(result);
    }

    return nullptr;
}

/**
 * 안전한 다운캐스팅을 수행합니다.
 * 상속 체인을 검사한 뒤, 유효하지 않으면 nullptr을 반환합니다.
 *
 * @tparam To 캐스팅할 대상 타입
 * @param obj 캐스팅할 객체의 포인터
 * @return 캐스팅에 성공하면 To* 포인터, 실패하면 nullptr
 */
template <typename To, IntrusiveReflectable From>
[[nodiscard]] To* Cast(From* obj)
{
    if (!obj)
    {
        return nullptr;
    }

    // From이 To를 상속 받았는지? (up-cast)
    if constexpr (std::derived_from<From, To>)
    {
        return static_cast<To*>(obj);
    }
    else
    {
        // obj가 원래 To였는지? (down-cast)
        if (IsA<To>(obj))
        {
            return reinterpret_cast<To*>(obj);
        }

        // 인터페이스 캐스팅 시도
        void* result = detail::CastToInterface(obj, obj->GetTypeId(), TypeId::Of<To>());
        return static_cast<To*>(result);
    }
}

template <typename To, IntrusiveReflectable From>
[[nodiscard]] const To* Cast(const From* obj)
{
    if (!obj)
    {
        return nullptr;
    }

    // From이 To를 상속 받았는지? (up-cast)
    if constexpr (std::derived_from<From, To>)
    {
        return static_cast<const To*>(obj);
    }
    else
    {
        // obj가 원래 To였는지? (down-cast)
        if (IsA<To>(obj))
        {
            return reinterpret_cast<const To*>(obj);
        }

        // 인터페이스 캐스팅 시도
        void* result = detail::CastToInterface(const_cast<From*>(obj), obj->GetTypeId(), TypeId::Of<To>());
        return static_cast<const To*>(result);
    }
}

/**
 * 다운캐스팅을 수행합니다.
 * 캐스팅에 실패하거나 nullptr이 전달되면 Assert가 발생합니다.
 * 릴리스 빌드에서는 검사 없이 static_cast를 수행합니다.
 *
 * @tparam To 캐스팅할 대상 타입
 * @param obj 캐스팅할 객체의 포인터 (nullptr 불가)
 * @return 캐스팅된 To* 포인터
 */
template <typename To, IntrusiveReflectable From>
[[nodiscard]] To* CastChecked(From* obj)
{
    SE_ASSERT(obj != nullptr, "CastChecked failed: Source pointer is null!");

    // 상속 기반 다운캐스팅 시도
    if (IsA<To>(obj))
    {
        return reinterpret_cast<To*>(obj);
    }

    // 인터페이스 캐스팅 시도
    void* result = detail::CastToInterface(obj, obj->GetTypeId(), TypeId::Of<To>());
    SE_ASSERT(
        result != nullptr,
        "CastChecked failed: Cannot cast '{}' to '{}'!",
        obj->GetTypeId().GetName(), TypeId::Of<To>().GetName()
    );
    return static_cast<To*>(result);
}

template <typename To, IntrusiveReflectable From>
[[nodiscard]] const To* CastChecked(const From* obj)
{
    SE_ASSERT(obj != nullptr, "CastChecked failed: Source pointer is null!");

    // 상속 기반 다운캐스팅 시도
    if (IsA<To>(obj))
    {
        return reinterpret_cast<const To*>(obj);
    }

    // 인터페이스 캐스팅 시도
    void* result = detail::CastToInterface(const_cast<From*>(obj), obj->GetTypeId(), TypeId::Of<To>());
    SE_ASSERT(
        result != nullptr,
        "CastChecked failed: Cannot cast '{}' to '{}'!",
        obj->GetTypeId().GetName(), TypeId::Of<To>().GetName()
    );
    return static_cast<const To*>(result);
}

/**
 * 런타임 타입이 정확히 To와 일치하는 경우에만 캐스팅합니다.
 * 상속 체인을 순회하지 않으므로, 파생 클래스는 매칭되지 않습니다.
 *
 * @tparam To 캐스팅할 대상 타입 (정확히 이 타입이어야 함)
 * @param obj 캐스팅할 객체의 포인터
 * @return 런타임 타입이 To와 동일하면 To* 포인터, 아니면 nullptr
 */
template <typename To, IntrusiveReflectable From>
[[nodiscard]] To* ExactCast(From* obj)
{
    if (obj && obj->GetTypeId() == TypeId::Of<To>())
    {
        return reinterpret_cast<To*>(obj);
    }
    return nullptr;
}

template <typename To, IntrusiveReflectable From>
[[nodiscard]] const To* ExactCast(const From* obj)
{
    if (obj && obj->GetTypeId() == TypeId::Of<To>())
    {
        return reinterpret_cast<const To*>(obj);
    }
    return nullptr;
}
} // namespace se
