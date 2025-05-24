export module SimpleEngine.Core.TypeTraits;

// static_assert에 사용되는 TypeTraits
export template <typename T>
inline constexpr bool TAlwaysFalse = false;
