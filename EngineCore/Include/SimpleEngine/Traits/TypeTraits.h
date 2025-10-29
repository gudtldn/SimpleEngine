#pragma once
#include <concepts>
#include <type_traits>

#include "SimpleEngine/Traits/FunctionTraits.h"


namespace se::traits
{
// static_assert에 사용되는 TypeTrait
template <typename T>
concept AlwaysFalse = false;

// Ts가 T와 같은지 확인하는 TypeTrait
template <typename T, typename... Ts>
concept IsAnyOf = (std::same_as<T, Ts> || ...);

// Ts가 T와 같은지 확인하는 TypeTrait (덜 엄격함)
template <typename T, typename... Ts>
concept IsAnyOfDecayed = IsAnyOf<std::decay_t<T>, std::decay_t<Ts>...>;

namespace details
{
template <typename T, typename... Us>
constexpr usize CountOccurrences = (std::same_as<std::decay_t<T>, std::decay_t<Us>> + ...);

template <typename Tuple, template <typename...> typename MapType>
struct TupleMapImpl;

template <template <typename...> typename TupleLike, template <typename...> typename MapType, typename... Ts>
struct TupleMapImpl<TupleLike<Ts...>, MapType>
{
    using Type = TupleLike<MapType<Ts>...>;
};

template <typename Tuple1, typename Tuple2>
struct IsDisjointImpl;

template <template <typename...> typename TupleLike1, template <typename...> typename TupleLike2, typename... Ts1, typename... Ts2>
struct IsDisjointImpl<TupleLike1<Ts1...>, TupleLike2<Ts2...>>
{
    static constexpr bool Value = (!IsAnyOfDecayed<Ts1, Ts2...> && ...);
};
}

// Ts...중에 중복된 타입이 존재하는지 확인
template <typename... Ts>
concept UniqueTypes = ((details::CountOccurrences<Ts, Ts...> == 1) && ...);

template <typename Tuple>
struct TupleHasUniqueTypesImpl;

template <template <typename...> typename TupleLike, typename... Ts>
struct TupleHasUniqueTypesImpl<TupleLike<Ts...>>
{
    static constexpr bool Value = UniqueTypes<Ts...>;
};

// TupleLike<Ts...>중에 중복된 타입이 존재하는지 확인
template <typename Tuple>
concept TupleHasUniqueTypes = TupleHasUniqueTypesImpl<Tuple>::Value;

// Tuple의 내부 타입에 MapType을 적용시킴
template <typename Tuple, template <typename...> typename MapType>
using TupleMap = details::TupleMapImpl<Tuple, MapType>::Type;

// 함수인지 확인하는 TypeTrait
template <typename T>
concept IsFunctionType = requires
{
    typename FunctionTraits<T>::Signature;
    typename FunctionTraits<T>::ReturnType;
};

// T 타입의 객체를 PrimaryTemplate<Args...> 패턴에 매칭되는지 확인합니다.
template <typename T, template <typename...> typename PrimaryTemplate>
concept IsSpecializationOf = requires
{
    []<typename... Args>(const PrimaryTemplate<Args...>&)
    {
    }(std::declval<T>());
};

/**
 * 두 개의 Tuple-like 타입이 서로소 집합인지, 즉 겹치는 멤버 타입을 하나도 가지지 않는지 확인
 */
template <typename Tuple1, typename Tuple2>
concept IsDisjoint = details::IsDisjointImpl<Tuple1, Tuple2>::Value;

// Ord 연산자가 구현되어 있는 타입
template <typename T>
concept OrderableType = requires(const T& a, const T& b)
{
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

// Eq 연산자가 구현되어 있는 타입
template <typename T>
concept ComparableType = requires(const T& a, const T& b)
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
}
