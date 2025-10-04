export module SE.Traits:TypeTraits;
import :FunctionTraits;

import std;


namespace se::traits::type_traits
{
// static_assert에 사용되는 TypeTrait
export template <typename T>
concept AlwaysFalse = false;

// Ts가 T와 같은지 확인하는 TypeTrait
export template <typename T, typename... Ts>
concept IsAnyOf = (std::same_as<T, Ts> || ...);

// Ts가 T와 같은지 확인하는 TypeTrait (덜 엄격함)
export template <typename T, typename... Ts>
concept IsAnyOfDecayed = IsAnyOf<std::decay_t<T>, std::decay_t<Ts>...>;

template <typename T, typename... Us>
constexpr size_t CountOccurrences = (std::same_as<std::decay_t<T>, std::decay_t<Us>> + ...);

// Ts...중에 중복된 타입이 존재하는지 확인
export template <typename... Ts>
concept UniqueTypes = ((CountOccurrences<Ts, Ts...> == 1) && ...);

template <typename Tuple>
struct TupleHasUniqueTypesImpl;

template <
    template <typename...> typename TupleLike,
    typename... Ts
>
struct TupleHasUniqueTypesImpl<TupleLike<Ts...>>
{
    static constexpr bool Value = UniqueTypes<Ts...>;
};

// TupleLike<Ts...>중에 중복된 타입이 존재하는지 확인
export template <typename Tuple>
concept TupleHasUniqueTypes = TupleHasUniqueTypesImpl<Tuple>::Value;

template <
    typename Tuple,
    template <typename...> typename MapType
>
struct TupleMapImpl;

template <
    template <typename...> typename TupleLike,
    template <typename...> typename MapType,
    typename... Ts
>
struct TupleMapImpl<TupleLike<Ts...>, MapType>
{
    using Type = TupleLike<MapType<Ts>...>;
};

// Tuple의 내부 타입에 MapType을 적용시킴
export template <
    typename Tuple,
    template <typename...> typename MapType
>
using TupleMap = TupleMapImpl<Tuple, MapType>::Type;

// 함수인지 확인하는 TypeTrait
export template <typename T>
concept IsFunctionType = requires
{
    typename func_traits::FunctionTraits<T>::Signature;
    typename func_traits::FunctionTraits<T>::ReturnType;
};

// T 타입의 객체를 PrimaryTemplate<Args...> 패턴에 매칭되는지 확인합니다.
export template <typename T, template <typename...> typename PrimaryTemplate>
concept IsSpecializationOf = requires
{
    []<typename... Args>(const PrimaryTemplate<Args...>&)
    {
    }(std::declval<T>());
};

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
 * 두 개의 Tuple-like 타입이 서로소 집합인지, 즉 겹치는 멤버 타입을 하나도 가지지 않는지 확인
 *
 * 타입 비교 시 내부적으로 IsAnyOfDecayed를 사용하므로, 참조(&, &&) 및 const/volatile 한정자는 무시됩니다.
 * @tparam Tuple1 첫 번째 Tuple-like 타입
 * @tparam Tuple2 두 번째 Tuple-like 타입
 */
export template <typename Tuple1, typename Tuple2>
concept IsDisjoint = IsDisjointImpl<Tuple1, Tuple2>::Value;

// Ord 연산자가 구현되어 있는 타입
export template <typename T>
concept OrderableType = requires(const T& a, const T& b)
{
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

// Eq 연산자가 구현되어 있는 타입
export template <typename T>
concept ComparableType = requires(const T& a, const T& b)
{
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

// 숫자 타입
export template <typename T>
concept NumberType = std::is_arithmetic_v<T>;

// 정수 타입
export template <typename T>
concept IntegralType = std::is_integral_v<T>;

// 실수 타입
export template <typename T>
concept FloatingType = std::is_floating_point_v<T>;
}
