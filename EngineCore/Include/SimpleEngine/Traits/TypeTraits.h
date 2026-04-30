#pragma once
#include <concepts>
#include <type_traits>

#include "SimpleEngine/Traits/FunctionTraits.h"


namespace se::traits
{
// static_assert에 사용되는 TypeTrait
template <typename...>
concept AlwaysFalse = false;

// Ts가 T와 같은지 확인하는 TypeTrait
template <typename T, typename... Ts>
concept IsAnyOf = (std::same_as<T, Ts> || ...);

// Ts가 T와 같은지 확인하는 TypeTrait (덜 엄격함)
template <typename T, typename... Ts>
concept IsAnyOfDecayed = IsAnyOf<std::decay_t<T>, std::decay_t<Ts>...>;

namespace detail
{
template <typename T, typename... Us>
constexpr usize CountOccurrences = (std::same_as<std::decay_t<T>, std::decay_t<Us>> + ...);

template <typename T>
struct ParamTypeImpl
{
    static constexpr bool USE_VALUE =
        (sizeof(T) <= sizeof(void*)) && std::is_trivially_copyable_v<T>;

    using Type = std::conditional_t<USE_VALUE, T, const T&>;
};

template <bool IsConst, typename T>
struct CopyConstImpl { using Type = std::conditional_t<IsConst, const T, T>; };

template <bool IsConst, typename T>
struct CopyConstImpl<IsConst, T*> { using Type = std::conditional_t<IsConst, const T*, T*>; };

template <bool IsConst, typename T>
struct CopyConstImpl<IsConst, T&> { using Type = std::conditional_t<IsConst, const T&, T&>; };

template <bool IsConst, typename T>
struct CopyConstImpl<IsConst, T&&> { using Type = std::conditional_t<IsConst, const T&&, T&&>; };
} // namespace detail

// Ts...중에 중복된 타입이 존재하는지 확인합니다.
template <typename... Ts>
concept UniqueTypePack = ((detail::CountOccurrences<Ts, Ts...> == 1) && ...);

/**
 * Self (객체 매개변수)의 const qualifier 상태를 기반으로 ReturnType의 최종 타입을 결정합니다.
 *
 * - Self가 const 객체이면, ReturnType에도 const를 적용합니다.
 * - ReturnType이 포인터일 경우, 포인터가 가리키는 대상에 const를 적용합니다 (예: const T*).
 * - ReturnType이 포인터가 아닐 경우, ReturnType 자체에 const를 적용합니다 (예: const T).
 */
template <typename Self, typename ReturnType>
    requires std::is_class_v<std::remove_reference_t<Self>>
using CopyConst = detail::CopyConstImpl<std::is_const_v<std::remove_reference_t<Self>>, ReturnType>::Type;

/**
 * 타입 T에 대한 최적의 전달 방식을 결정합니다.
 * - 크기가 포인터 이하이고 복사 비용이 저렴한 경우: T
 * - 그 외의 경우 (큰 객체, 복잡한 클래스 등): const T&
 */
template <typename T>
using ParamType = detail::ParamTypeImpl<T>::Type;

// 함수인지 확인하는 TypeTrait
template <typename T>
concept FunctionType = requires
{
    typename FunctionTraits<std::remove_cvref_t<T>>::Signature;
    typename FunctionTraits<std::remove_cvref_t<T>>::ReturnType;
};

// T 타입의 객체를 PrimaryTemplate<Args...> 패턴에 매칭되는지 확인합니다.
template <typename T, template <typename...> typename PrimaryTemplate>
concept IsSpecializationOf = requires
{
    []<typename... Args>(const PrimaryTemplate<Args...>&)
    {
    }(std::declval<T>());
};

// Ord 연산자가 구현되어 있는 타입
template <typename T>
concept Orderable = requires(const T& a, const T& b)
{
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

// Eq 연산자가 구현되어 있는 타입
template <typename T>
concept Comparable = requires(const T& a, const T& b)
{
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

// 숫자 타입
template <typename T>
concept NumberType = std::is_arithmetic_v<T>;

// 정수 타입
template <typename T>
concept IntegralType = std::is_integral_v<T>;

// 실수 타입
template <typename T>
concept FloatingType = std::is_floating_point_v<T>;

// Enum
template <typename T>
concept EnumType = std::is_enum_v<T>;
} // namespace se::traits
