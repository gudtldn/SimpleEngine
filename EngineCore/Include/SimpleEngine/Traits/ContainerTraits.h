#pragma once
#include <type_traits>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
// Forward declarations
template <typename T, usize N> class FixedArray;
template <typename T, typename Allocator> class Array;
template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator> class HashMap;
template <typename T, typename Hasher, typename KeyEq, typename Allocator> class HashSet;
template <typename Key, typename Value, typename Pred, typename Allocator> class Map;
template <typename T, typename Pred, typename Allocator> class Set;
template <typename Key, typename Value, typename Pred, typename Allocator> class FlatMap;
template <typename T, typename Pred, typename Allocator> class FlatSet;
} // namespace se


namespace se::traits
{
namespace detail
{
// 컨테이너 판별용 내부 구현
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
template <typename T>                               struct ElementTypeImpl                         { using Type = void; };
template <typename T, usize N>                      struct ElementTypeImpl<FixedArray<T, N>>       { using Type = T;    };
template <typename T, typename... Args>             struct ElementTypeImpl<Array<T, Args...>>      { using Type = T;    };
template <typename T, typename... Args>             struct ElementTypeImpl<HashSet<T, Args...>>    { using Type = T;    };
template <typename T, typename... Args>             struct ElementTypeImpl<Set<T, Args...>>        { using Type = T;    };
template <typename T, typename... Args>             struct ElementTypeImpl<FlatSet<T, Args...>>    { using Type = T;    };

template <typename T>                               struct MapKeyValueImpl                         { using KeyType = void; using ValueType = void; };
template <typename K, typename V, typename... Args> struct MapKeyValueImpl<HashMap<K, V, Args...>> { using KeyType = K;    using ValueType = V;    };
template <typename K, typename V, typename... Args> struct MapKeyValueImpl<Map<K, V, Args...>>     { using KeyType = K;    using ValueType = V;    };
template <typename K, typename V, typename... Args> struct MapKeyValueImpl<FlatMap<K, V, Args...>> { using KeyType = K;    using ValueType = V;    };
} // namespace detail

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

/** 컨테이너의 요소(Element) 타입을 추출합니다. */
template <typename T>
using ElementOf = detail::ElementTypeImpl<std::remove_cvref_t<T>>::Type;

/** Map-like 컨테이너의 Key 타입을 추출합니다. */
template <typename T>
using KeyOf = detail::MapKeyValueImpl<std::remove_cvref_t<T>>::KeyType;

/** Map-like 컨테이너의 Value 타입을 추출합니다. */
template <typename T>
using ValueOf = detail::MapKeyValueImpl<std::remove_cvref_t<T>>::ValueType;
}  // namespace se::traits
