#pragma once

#include "SimpleEngine/Traits/TypeTraits.h"

#include <tuple>
#include <utility>


namespace se::traits
{
namespace detail
{
/**
 * TupleLike의 Ts...를 템플릿 인자로 추출해 제네릭 람다에 전달하기 위한 구현체
 */
template <typename Signature>
struct ApplyTypesImpl;

template <template <typename...> typename TupleLike, typename... Ts>
struct ApplyTypesImpl<TupleLike<Ts...>>
{
    template <typename Fn>
    static constexpr auto Apply(Fn&& func)
    {
        return std::forward<Fn>(func).template operator()<Ts...>();
    }
};

template <typename R, typename... Ts>
struct ApplyTypesImpl<R(Ts...)>
{
    template <typename Fn>
    static constexpr auto Apply(Fn&& func)
    {
        return std::forward<Fn>(func).template operator()<Ts...>();
    }
};

/**
 * 튜플 타입의 파라미터 팩을 다른 템플릿에 바인딩하기 위한 구현체
 */
template <typename Tuple>
struct RebindImpl;

template <template <typename...> typename TupleLike, typename... Ts>
struct RebindImpl<TupleLike<Ts...>>
{
    template <template <typename...> typename Target>
    using To = Target<Ts...>;
};

template <typename R, typename... Ts>
struct RebindImpl<R(Ts...)>
{
    template <template <typename...> typename Target>
    using To = Target<Ts...>;
};

/**
 * 두 개의 TupleLike 타입을 병합하기 위한 구현체
 */
template <typename T1, typename T2>
struct MergeTwoTuplesImpl;

template <
    template<typename...> typename TupleLike,
    typename... Ts1,
    typename... Ts2
>
struct MergeTwoTuplesImpl<TupleLike<Ts1...>, TupleLike<Ts2...>>
{
    using Type = TupleLike<Ts1..., Ts2...>;
};

/**
 * 여러 튜플 타입을 재귀적으로 연결하기 위한 구현체
 */
template <typename...>
struct TupleCatImpl;

template <>
struct TupleCatImpl<>
{
    using Type = std::tuple<>;
};

template <typename T1>
struct TupleCatImpl<T1>
{
    using Type = T1;
};

template <typename T1, typename T2, typename... Rest>
struct TupleCatImpl<T1, T2, Rest...>
{
private:
    using MergedFirstTwo = MergeTwoTuplesImpl<T1, T2>::Type;

public:
    using Type = TupleCatImpl<MergedFirstTwo, Rest...>::Type;
};

/**
 * 중첩된 TupleLike 타입을 지정된 컨테이너로 평탄화하기 위한 구현체
 */
template <template <typename...> typename ResultTupleLike, typename T>
struct FlattenTupleImpl
{
    using Type = ResultTupleLike<T>;
};

template <
    template <typename...> typename ResultTupleLike,
    template <typename...> typename InputTupleLike,
    typename... Ts
>
struct FlattenTupleImpl<ResultTupleLike, InputTupleLike<Ts...>>
{
    using Type = TupleCatImpl<typename FlattenTupleImpl<ResultTupleLike, Ts>::Type...>::Type;
};

/**
 * Tuple의 각 타입에 대해 Trait을 일괄 적용하기 위한 구현체
 */
template <typename Tuple, template <typename...> typename Trait>
struct TupleMapImpl;

template <template <typename...> typename TupleLike, template <typename...> typename Trait, typename... Ts>
struct TupleMapImpl<TupleLike<Ts...>, Trait>
{
    using Type = TupleLike<Trait<Ts>...>;
};

/**
 * 두 TupleLike 타입이 겹치는 타입을 가지지 않는지 확인하기 위한 구현체
 */
template <typename Tuple1, typename Tuple2>
struct IsDisjointImpl;

template <
    template <typename...> typename TupleLike1,
    template <typename...> typename TupleLike2,
    typename... Ts1,
    typename... Ts2
>
struct IsDisjointImpl<TupleLike1<Ts1...>, TupleLike2<Ts2...>>
{
    static constexpr bool Value = (!IsAnyOfDecayed<Ts1, Ts2...> && ...);
};

/**
 * TupleLike의 내부 타입이 모두 고유한지 확인하기 위한 구현체
 */
template <typename Tuple>
struct TupleUniqueTypesImpl;

template <template <typename...> typename TupleLike, typename... Ts>
struct TupleUniqueTypesImpl<TupleLike<Ts...>>
{
    static constexpr bool Value = UniqueTypePack<Ts...>;
};
} // namespace detail


/**
 * TupleLike 타입에서 Ts...를 추출해 제네릭 람다의 템플릿 파라미터로 전달합니다.
 * std::apply와 달리 런타임 값이 아닌 타입을 전달합니다.
 *
 * @tparam TupleLike 타입들을 추출할 TupleLike (예: std::tuple<int, float>)
 * @tparam Fn 템플릿 operator()를 가진 제네릭 람다 또는 Functor 타입
 * @param func 추출된 타입들을 템플릿 인자로 받아 호출될 객체
 *
 * @code
 * using MyTuple = std::tuple<int, float>;
 * ApplyTypes<MyTuple>([]<typename... Ts>()
 * {
 *     // Ts = int, float
 * });
 * @endcode
 */
template <typename TupleLike, typename Fn>
constexpr auto ApplyTypes(Fn&& func)
    requires requires { detail::ApplyTypesImpl<TupleLike>::Apply(std::forward<Fn>(func)); }
{
    return detail::ApplyTypesImpl<TupleLike>::Apply(std::forward<Fn>(func));
}

/**
 * 튜플 타입의 파라미터 팩을 다른 템플릿 컨테이너로 리바인딩(rebind)합니다.
 *
 * @code
 * using StdTuple = std::tuple<int, float>;
 * Rebind<StdTuple, MyTuple>; // -> MyTuple<int, float>
 * @endcode
 */
template <
    typename Tuple,
    template <typename...> typename Target
>
using Rebind = detail::RebindImpl<Tuple>::template To<Target>;

/**
 * 여러 튜플 타입의 멤버 타입들을 모두 포함하는 단일 튜플 타입을 만듭니다.
 */
template <typename... Tuples>
    requires (IsSpecializationOf<Tuples, std::tuple> && ...)
using TupleCat = detail::TupleCatImpl<Tuples...>::Type;

/**
 * 중첩된 TupleLike 타입을 지정된 컨테이너(기본값: std::tuple)로 평탄화합니다.
 *
 * @code
 * // std::tuple<A, std::tuple<B, C>> -> std::tuple<A, B, C>
 * FlattenTuple<std::tuple<A, std::tuple<B, C>>>;
 * @endcode
 */
template <typename T, template <typename...> typename ResultTupleLike = std::tuple>
using FlattenTuple = detail::FlattenTupleImpl<ResultTupleLike, T>::Type;

/**
 * Tuple의 내부 타입에 대해 Trait을 일괄 적용합니다.
 *
 * @code
 * // std::tuple<int, float> -> std::tuple<const int, const float>
 * TupleMap<std::tuple<int, float>, std::add_const_t>;
 * @endcode
 */
template <typename Tuple, template <typename...> typename Trait>
using TupleMap = detail::TupleMapImpl<Tuple, Trait>::Type;

/**
 * 두 개의 TupleLike 타입이 겹치는 멤버 타입을 가지지 않는지 확인합니다.
 */
template <typename Tuple1, typename Tuple2>
concept IsDisjoint = detail::IsDisjointImpl<Tuple1, Tuple2>::Value;

/**
 * TupleLike의 내부 타입이 모두 고유한지 확인합니다.
 */
template <typename Tuple>
concept TupleUniqueTypes = detail::TupleUniqueTypesImpl<Tuple>::Value;
} // namespace se::traits
