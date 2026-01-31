#pragma once

#include <tuple>
#include <type_traits>

#include "SimpleEngine/Traits/TypeTraits.h"


namespace se::utility
{
namespace detail
{
template <typename Signature>
struct WithUnpackedTypesImpl;

/**
 * TupleLike에 포함된 모든 타입을 템플릿 파라미터 팩 `Ts...`로 추출(unpack)합니다.
 * @tparam Ts TupleLike으로부터 추출된 타입 파라미터 팩
 */
template <template <typename...> typename TupleLike, typename... Ts>
struct WithUnpackedTypesImpl<TupleLike<Ts...>>
{
    template <typename Fn>
    static constexpr auto Unpack(Fn&& func)
    {
        return std::forward<Fn>(func).template operator()<Ts...>();
    }
};

/**
 * 함수 Signature에 포함된 모든 타입을 템플릿 파라미터 팩 `Ts...`로 추출(unpack)합니다.
 * @tparam Ts 함수 Signature로부터 추출된 타입 파라미터 팩
 */
template <typename R, typename... Ts>
struct WithUnpackedTypesImpl<R(Ts...)>
{
    template <typename Fn>
    static constexpr auto Unpack(Fn&& func)
    {
        return std::forward<Fn>(func).template operator()<Ts...>();
    }
};

/**
 * 템플릿 파라미터 팩을 다른 템플릿 컨테이너로 리바인딩(rebind)하는 기능을 제공합니다.
 * @tparam Tuple 템플릿 파라미터 팩을 추출할 원본 템플릿 타입입니다. (예: `std::tuple<int, float>`)
 */
template <typename Tuple>
struct RebindImpl;

template <
    template <typename...> typename TupleLike,
    typename... Ts
>
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
 * 두 개의 튜플 타입 T1과 T2를 하나의 튜플 타입으로 병합합니다.
 */
template <typename T1, typename T2>
struct MergeTwoTuples;

template <
    template<typename...> typename TupleLike,
    typename... Ts1,
    typename... Ts2
>
struct MergeTwoTuples<TupleLike<Ts1...>, TupleLike<Ts2...>>
{
    // T1의 타입 목록(Ts1...)과 T2의 타입 목록(Ts2...)을 합쳐 새로운 튜플 타입을 정의
    using Type = TupleLike<Ts1..., Ts2...>;
};

/**
 * 여러 튜플 타입을 재귀적으로 병합하기 위한 구현체
 */
template <typename...>
struct TupleCatImpl;

// 기본 케이스: 튜플에 멤버가 하나도 없으면 빈 튜플을 반환
template <>
struct TupleCatImpl<>
{
    using Type = std::tuple<>;
};

// 기본 케이스: 튜플이 하나만 남으면 그 자신을 결과 타입으로 가짐
template <typename T1>
struct TupleCatImpl<T1>
{
    using Type = T1;
};

// 재귀 케이스: 튜플이 두 개 이상일 때
template <typename T1, typename T2, typename... Rest>
struct TupleCatImpl<T1, T2, Rest...>
{
private:
    // 먼저 앞의 두 튜플을 병합
    using MergedFirstTwo = MergeTwoTuples<T1, T2>::Type;

public:
    // 병합된 결과와 나머지 튜플(Rest...)로 다시 재귀 호출
    using Type = TupleCatImpl<MergedFirstTwo, Rest...>::Type;
};

/**
 * TupleLike 타입을 평탄화하기 위한 구현체
 * @tparam ResultTupleLike 최종 결과물로 사용할 튜플 컨테이너 타입 (예: std::tuple)
 * @tparam T 평탄화할 대상 타입
 */
template <template <typename...> typename ResultTupleLike, typename T>
struct FlattenTupleImpl
{
    // 기본 케이스: T가 튜플이 아닐 때, ResultTupleLike<T>를 반환.
    using Type = ResultTupleLike<T>;
};

template <
    template <typename...> typename ResultTupleLike,
    template <typename...> typename InputTupleLike,
    typename... Ts
>
struct FlattenTupleImpl<ResultTupleLike, InputTupleLike<Ts...>>
{
    // 재귀 케이스: T가 튜플일 때,
    // 각 멤버에 대해 재귀 호출(결과 타입은 ResultTupleLike로 고정)하고, 그 결과들을 TupleCat으로 합침
    using Type = TupleCatImpl<typename FlattenTupleImpl<ResultTupleLike, Ts>::Type...>::Type;
};
}

/**
 * 튜플 타입(`TupleLike`)에서 타입 목록을 추출하여 제네릭 호출 가능 객체(`func`)에 템플릿 인자로 전달합니다.
 * @note std::apply와는 다르게, 인자를 받지 않습니다.
 *
 * @tparam TupleLike 타입들을 추출할 TupleLike (ex: std::tuple<...>)
 * @tparam Fn 템플릿 `operator()`를 가진 제네릭 Lambda 또는 Functor 타입
 * @param func 추출된 타입들을 템플릿 인자로 받아 호출될 객체
 * @return `func`를 호출한 결과값을 그대로 반환
 *
 * @code
 * using MyTuple = std::tuple<int, std::string, double>;
 * auto result = UnpackTuple<MyTuple>([]<typename... TArgs>()
 * {
 *     // 이 블록은 TArgs = int, std::string, double 로 호출됩니다.
 *     return sizeof...(TArgs); // 3을 반환
 * });
 * @endcode
 */
template <typename TupleLike, typename Fn>
constexpr auto WithUnpackedTypes(Fn&& func)
    requires requires { detail::WithUnpackedTypesImpl<TupleLike>::Unpack(std::forward<Fn>(func)); }
{
    return detail::WithUnpackedTypesImpl<TupleLike>::Unpack(std::forward<Fn>(func));
}

/**
 * 템플릿 파라미터 팩을 다른 템플릿 컨테이너로 리바인딩(rebind)합니다.
 * @tparam Tuple 리바인딩(rebind)할 튜플 타입
 * @code
 * template <typename... Ts> struct MyTuple {};
 * using StdTuple = std::tuple<int, float>;
 * Rebind<StdTuple, MyTuple>; // MyTuple<int, float>
 * @endcode
 */
template <
    typename Tuple,
    template <typename...> typename Target
>
using Rebind = detail::RebindImpl<Tuple>::template To<Target>;

/**
 * 여러 튜플 타입의 멤버 타입들을 모두 포함하는 단일 튜플 타입을 만듭니다.
 * @tparam Tuples 병합할 튜플 타입들
 */
template <typename... Tuples>
    requires (traits::IsSpecializationOf<Tuples, std::tuple> && ...)
using TupleCat = detail::TupleCatImpl<Tuples...>::Type;

/**
 * 중첩된 TupleLike 타입을 지정된 컨테이너(기본값: std::tuple)로 평탄화합니다.
 * @tparam T 평탄화할 타입
 * @tparam ResultTupleLike (선택) 결과물로 사용할 튜플 컨테이너.
 *         예: FlattenTuple<MyTuple<...>, MyTuple>
 */
template <typename T, template <typename...> typename ResultTupleLike = std::tuple>
using FlattenTuple = detail::FlattenTupleImpl<ResultTupleLike, T>::Type;
}
