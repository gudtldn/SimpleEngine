#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <concepts>
#include <type_traits>
#include <utility>


namespace se::traits
{
/**
 * 단순 타입을 리스팅하는 타입입니다.
 * 만약 값이 필요하면 Rebind로 std::tuple에 바인딩하세요.
 */
template <typename... Ts>
struct TypeList {};

namespace detail
{
/** Us... 중 T와 같은 타입(remove_cvref 기준)의 개수 */
template <typename T, typename... Us>
inline constexpr usize COUNT_OCCURRENCES =
    (usize{ 0 } + ... + static_cast<usize>(std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<Us>>));

/**
 * 목록의 원소 개수를 구하기 위한 구현체
 */
template <typename List>
struct SizeImpl;

template <template <typename...> typename ListLike, typename... Ts>
struct SizeImpl<ListLike<Ts...>>
{
    static constexpr usize Value = sizeof...(Ts);
};

/**
 * 목록에 타입이 들어 있는지 확인하기 위한 구현체
 */
template <typename List, typename T>
struct ContainsImpl;

template <template <typename...> typename ListLike, typename... Ts, typename T>
struct ContainsImpl<ListLike<Ts...>, T>
{
    static constexpr bool Value = IsAnyOfUnqualified<T, Ts...>;
};

/**
 * 목록에서 타입의 위치를 찾기 위한 구현체
 */
template <typename List, typename T>
struct IndexOfImpl;

template <template <typename...> typename ListLike, typename... Ts, typename T>
struct IndexOfImpl<ListLike<Ts...>, T>
{
    static_assert(COUNT_OCCURRENCES<T, Ts...> == 1, "IndexOf requires the type to appear exactly once in the list.");

    static constexpr usize Value = []
    {
        usize index = 0;
        usize current = 0;
        ((std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<Ts>> ? (void)(index = current) : (void)0, ++current), ...);
        return index;
    }();
};

/**
 * 목록의 Ts...를 템플릿 인자로 추출해 제네릭 람다에 전달하기 위한 구현체
 */
template <typename List>
struct ApplyTypesImpl;

template <template <typename...> typename ListLike, typename... Ts>
struct ApplyTypesImpl<ListLike<Ts...>>
{
    template <typename Fn>
    static constexpr auto Apply(Fn&& func)
    {
        return std::forward<Fn>(func).template operator()<Ts...>();
    }
};

/**
 * 목록의 파라미터 팩을 다른 템플릿에 바인딩하기 위한 구현체
 */
template <typename List>
struct RebindImpl;

template <template <typename...> typename ListLike, typename... Ts>
struct RebindImpl<ListLike<Ts...>>
{
    template <template <typename...> typename Target>
    using To = Target<Ts...>;
};

/**
 * 두 목록을 병합하기 위한 구현체
 */
template <typename L1, typename L2>
struct MergeTwoImpl;

template <
    template <typename...> typename ListLike,
    typename... Ts1,
    typename... Ts2
>
struct MergeTwoImpl<ListLike<Ts1...>, ListLike<Ts2...>>
{
    using Type = ListLike<Ts1..., Ts2...>;
};

/**
 * 여러 목록을 재귀적으로 연결하기 위한 구현체
 */
template <typename...>
struct ConcatImpl;

template <>
struct ConcatImpl<>
{
    using Type = TypeList<>;
};

template <typename L1>
struct ConcatImpl<L1>
{
    using Type = L1;
};

template <typename L1, typename L2, typename... Rest>
struct ConcatImpl<L1, L2, Rest...>
{
private:
    using MergedFirstTwo = MergeTwoImpl<L1, L2>::Type;

public:
    using Type = ConcatImpl<MergedFirstTwo, Rest...>::Type;
};

/**
 * 중첩된 TypeList를 하나의 TypeList로 평탄화하기 위한 구현체
 */
template <typename T>
struct FlattenImpl
{
    using Type = TypeList<T>;
};

template <typename... Ts>
struct FlattenImpl<TypeList<Ts...>>
{
    using Type = ConcatImpl<typename FlattenImpl<Ts>::Type...>::Type;
};

/**
 * 목록의 각 타입에 대해 Trait을 일괄 적용하기 위한 구현체
 */
template <typename List, template <typename...> typename Trait>
struct MapImpl;

template <template <typename...> typename ListLike, template <typename...> typename Trait, typename... Ts>
struct MapImpl<ListLike<Ts...>, Trait>
{
    using Type = ListLike<Trait<Ts>...>;
};

/**
 * 두 목록이 겹치는 타입을 가지지 않는지 확인하기 위한 구현체
 */
template <typename L1, typename L2>
struct IsDisjointImpl;

template <
    template <typename...> typename ListLike1,
    template <typename...> typename ListLike2,
    typename... Ts1,
    typename... Ts2
>
struct IsDisjointImpl<ListLike1<Ts1...>, ListLike2<Ts2...>>
{
    static constexpr bool Value = (!IsAnyOfUnqualified<Ts1, Ts2...> && ...);
};

/**
 * 목록의 타입이 모두 고유한지 확인하기 위한 구현체
 */
template <typename List>
struct IsUniqueImpl;

template <template <typename...> typename ListLike, typename... Ts>
struct IsUniqueImpl<ListLike<Ts...>>
{
    static constexpr bool Value = ((COUNT_OCCURRENCES<Ts, Ts...> == 1) && ...); // NOLINT(*-redundant-parentheses)
};
} // namespace detail


/** 목록의 원소 개수입니다. */
template <typename List>
inline constexpr usize Size = detail::SizeImpl<List>::Value;

/** 목록에 T가 들어 있는지 확인합니다. cv와 참조를 제거한 타입으로 비교합니다. */
template <typename List, typename T>
concept Contains = detail::ContainsImpl<List, T>::Value;

/** 목록에서 T의 위치입니다. cv와 참조를 제거한 타입으로 비교하며, T가 정확히 한 번 있어야 합니다. */
template <typename List, typename T>
inline constexpr usize IndexOf = detail::IndexOfImpl<List, T>::Value;

/**
 * 목록에서 Ts...를 추출해 제네릭 람다의 템플릿 파라미터로 전달합니다.
 * std::apply와 달리 런타임 값이 아닌 타입을 전달합니다.
 *
 * @tparam List 타입들을 추출할 목록 (예: TypeList<i32, f32>)
 * @tparam Fn 템플릿 operator()를 가진 제네릭 람다 또는 Functor 타입
 * @param func 추출된 타입들을 템플릿 인자로 받아 호출될 객체
 *
 * @code
 * using MyList = TypeList<i32, f32>;
 * ApplyTypes<MyList>([]<typename... Ts>()
 * {
 *     // Ts = i32, f32
 * });
 * @endcode
 */
template <typename List, typename Fn>
constexpr auto ApplyTypes(Fn&& func)
    requires requires { detail::ApplyTypesImpl<List>::Apply(std::forward<Fn>(func)); }
{
    return detail::ApplyTypesImpl<List>::Apply(std::forward<Fn>(func));
}

/**
 * 목록의 파라미터 팩을 다른 템플릿으로 리바인딩(rebind)합니다.
 *
 * @code
 * Rebind<TypeList<i32, f32>, std::tuple>; // -> std::tuple<i32, f32>
 * @endcode
 */
template <
    typename List,
    template <typename...> typename Target
>
using Rebind = detail::RebindImpl<List>::template To<Target>;

/**
 * 여러 목록의 타입들을 모두 포함하는 단일 목록을 만듭니다. 인자가 없으면 TypeList<>입니다.
 */
template <typename... Lists>
using Concat = detail::ConcatImpl<Lists...>::Type;

/**
 * 중첩된 TypeList를 하나의 TypeList로 평탄화합니다. TypeList가 아닌 타입은 펼치지 않습니다.
 *
 * @code
 * // TypeList<A, TypeList<B, C>, Optional<D>> -> TypeList<A, B, C, Optional<D>>
 * Flatten<TypeList<A, TypeList<B, C>, Optional<D>>>;
 * @endcode
 */
template <typename List>
using Flatten = detail::FlattenImpl<List>::Type;

/**
 * 목록의 각 타입에 대해 Trait을 일괄 적용합니다.
 *
 * @code
 * // TypeList<i32, f32> -> TypeList<const i32, const f32>
 * Map<TypeList<i32, f32>, std::add_const_t>;
 * @endcode
 */
template <typename List, template <typename...> typename Trait>
using Map = detail::MapImpl<List, Trait>::Type;

/** 두 목록이 겹치는 타입을 가지지 않는지 확인합니다. cv와 참조를 제거한 타입으로 비교합니다. */
template <typename L1, typename L2>
concept IsDisjoint = detail::IsDisjointImpl<L1, L2>::Value;

/** 목록의 타입이 모두 고유한지 확인합니다. cv와 참조를 제거한 타입으로 비교합니다. */
template <typename List>
concept IsUnique = detail::IsUniqueImpl<std::remove_cvref_t<List>>::Value;
} // namespace se::traits
