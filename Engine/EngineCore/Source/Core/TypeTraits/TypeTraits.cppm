export module SimpleEngine.Core.TypeTraits;

import std;


// static_assert에 사용되는 Type Trait
export template <typename T>
inline constexpr bool TAlwaysFalse = false;

// Ts가 T와 같은지 확인하는 Type Trait
export template <typename T, typename... Ts>
constexpr bool TIsAnyOf = (std::same_as<T, Ts> || ...);
