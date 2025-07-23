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

export template <typename T>
concept IntegralType = std::is_integral_v<T>;

export template <typename T>
concept FloatingType = std::is_floating_point_v<T>;
}
