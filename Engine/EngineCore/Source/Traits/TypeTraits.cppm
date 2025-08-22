export module SimpleEngine.Traits:TypeTraits;
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
