export module SimpleEngine.TypeTraits;

import std;


namespace se::type_traits
{
// static_assert에 사용되는 Type Trait
export template <typename T>
inline constexpr bool TAlwaysFalse = false;

// Ts가 T와 같은지 확인하는 Type Trait
export template <typename T, typename... Ts>
concept TIsAnyOf = (std::same_as<T, Ts> || ...);

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
