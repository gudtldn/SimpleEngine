#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <type_traits>


// Forward declarations
namespace se
{
template <typename T> class Optional;
template <typename T, usize N> class FixedArray;
template <typename T, typename Allocator> class Array;
template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator> class HashMap;
template <typename T, typename Hasher, typename KeyEq, typename Allocator> class HashSet;
template <typename Key, typename Value, typename Pred, typename Allocator> class Map;
template <typename T, typename Pred, typename Allocator> class Set;
template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer> class FlatMap;
template <typename T, typename Pred, typename Container> class FlatSet;
} // namespace se


namespace se::traits
{
namespace detail
{
// 컨테이너 판별용 내부 구현
template <typename T>                               constexpr bool IsOptionalImpl                        = false;
template <typename T>                               constexpr bool IsOptionalImpl<Optional<T>>           = true;

template <typename T>                               constexpr bool IsArrayLikeImpl                       = false;
template <typename T, usize N>                      constexpr bool IsArrayLikeImpl<FixedArray<T, N>>     = true;
template <typename T, typename... Args>             constexpr bool IsArrayLikeImpl<Array<T, Args...>>    = true;

template <typename T>                               constexpr bool IsMapLikeImpl                         = false;
template <typename K, typename V, typename... Args> constexpr bool IsMapLikeImpl<HashMap<K, V, Args...>> = true;
template <typename K, typename V, typename... Args> constexpr bool IsMapLikeImpl<Map<K, V, Args...>>     = true;
template <typename K, typename V, typename... Args> constexpr bool IsMapLikeImpl<FlatMap<K, V, Args...>> = true;

template <typename T>                               constexpr bool IsSetLikeImpl                         = false;
template <typename T, typename... Args>             constexpr bool IsSetLikeImpl<HashSet<T, Args...>>    = true;
template <typename T, typename... Args>             constexpr bool IsSetLikeImpl<Set<T, Args...>>        = true;
template <typename T, typename... Args>             constexpr bool IsSetLikeImpl<FlatSet<T, Args...>>    = true;

// 타입 추출용 내부 구현
template <typename T>
concept HasInnerType = requires { typename T::InnerType; };

template <typename T>                               struct InnerTypeImpl                      { using Type = void;         };
template <HasInnerType T>                           struct InnerTypeImpl<T>                   { using Type = T::InnerType; };
template <typename T>                               struct InnerTypeImpl<Optional<T>>         { using Type = T;            };
template <typename T, usize N>                      struct InnerTypeImpl<FixedArray<T, N>>    { using Type = T;            };
template <typename T, typename... Args>             struct InnerTypeImpl<Array<T, Args...>>   { using Type = T;            };
template <typename T, typename... Args>             struct InnerTypeImpl<HashSet<T, Args...>> { using Type = T;            };
template <typename T, typename... Args>             struct InnerTypeImpl<Set<T, Args...>>     { using Type = T;            };
template <typename T, typename... Args>             struct InnerTypeImpl<FlatSet<T, Args...>> { using Type = T;            };

template <typename T>
struct InnerOfImpl
{
    using ExtractedType = InnerTypeImpl<std::remove_cvref_t<T>>::Type;
    using Type = std::conditional_t<std::is_void_v<ExtractedType>, T, ExtractedType>;
};

template <typename T>                               struct MapKeyValueImpl                         { using KeyType = void; using ValueType = void; };
template <typename K, typename V, typename... Args> struct MapKeyValueImpl<HashMap<K, V, Args...>> { using KeyType = K;    using ValueType = V;    };
template <typename K, typename V, typename... Args> struct MapKeyValueImpl<Map<K, V, Args...>>     { using KeyType = K;    using ValueType = V;    };
template <typename K, typename V, typename... Args> struct MapKeyValueImpl<FlatMap<K, V, Args...>> { using KeyType = K;    using ValueType = V;    };
} // namespace detail

template <typename T> concept OptionalLike  = detail::IsOptionalImpl<std::remove_cvref_t<T>>;
template <typename T> concept ArrayLike     = detail::IsArrayLikeImpl<std::remove_cvref_t<T>>;
template <typename T> concept MapLike       = detail::IsMapLikeImpl<std::remove_cvref_t<T>>;
template <typename T> concept SetLike       = detail::IsSetLikeImpl<std::remove_cvref_t<T>>;
template <typename T> concept ContainerLike = ArrayLike<T> || MapLike<T> || SetLike<T>;


/** Reserve()를 지원하는 컨테이너인지 확인합니다. */
template <typename T>
concept Reservable = requires(T& container, usize size)
{
    container.Reserve(size);
};

/** Resize(), ResizeUninitialized()를 지원하는 컨테이너인지 확인합니다. */
template <typename T>
concept Resizable = requires(T& container, usize size) { container.Resize(size); }
    || requires(T& container, usize size) { container.ResizeUninitialized(size); };


/**
 * 주어진 타입 T의 내부(Wrapped/Element) 타입을 추출합니다.
 *
 * 엔진 내장 컨테이너(Optional, Array 등)이거나 내부에 `InnerType` 타입 별칭을 가진 객체라면 해당 내부 타입을 반환합니다.
 * 내부 타입을 추출할 수 없는 일반 타입인 경우, T 자기 자신을 그대로 반환합니다.
 */
template <typename T>
using InnerOf = detail::InnerOfImpl<T>::Type;

/** Map-like 컨테이너의 Key 타입을 추출합니다. */
template <typename T>
    requires MapLike<T>
using KeyOf = detail::MapKeyValueImpl<std::remove_cvref_t<T>>::KeyType;

/** Map-like 컨테이너의 Value 타입을 추출합니다. */
template <typename T>
    requires MapLike<T>
using ValueOf = detail::MapKeyValueImpl<std::remove_cvref_t<T>>::ValueType;
} // namespace se::traits
