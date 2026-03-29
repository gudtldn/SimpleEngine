#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Traits/TupleTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <tuple>
#include <type_traits>


namespace se
{
/**
 * 쿼리 결과에 반드시 포함되어야 하는 Component를 지정하는 FilterTag입니다.
 * @tparam Components 필터링할 Component 타입들
 */
template <typename... Components>
struct With
{
    using Types = std::tuple<Components...>;
};

/**
 * 쿼리 결과에서 반드시 제외되어야 하는 Component를 지정하는 FilterTag입니다.
 * @tparam Components 필터링할 Component 타입들
 */
template <typename... Components>
struct Without
{
    using Types = std::tuple<Components...>;
};

namespace detail
{
/** 타입 T가 FilterTag(With/Without)가 아닌 Fetch 대상인지 확인합니다. */
template <typename T>
concept IsFetchTag = !(traits::IsSpecializationOf<T, With> || traits::IsSpecializationOf<T, Without>);

/** Optional이나 Entity가 아닌 필수 컴포넌트인지 확인합니다. */
template <typename T>
concept IsRequiredComponent = IsFetchTag<T> && !(traits::IsSpecializationOf<T, Optional> || std::same_as<T, Entity>);

template <typename TupleType>
struct TupleContainsPointersImpl;

template <
    template <typename...> typename TupleLike,
    typename... Ts
>
struct TupleContainsPointersImpl<TupleLike<Ts...>>
{
    static constexpr bool Value = (std::is_pointer_v<Ts> || ...);
};

template <typename TupleType>
concept TupleContainsPointers = TupleContainsPointersImpl<TupleType>::Value;
} // namespace detail

template <typename... Ts>
concept QueryParameterPack =
    sizeof...(Ts) > 0                                                                                           // Ts...의 개수는 1개 이상
    && traits::TupleUniqueTypes<traits::FlattenTuple<std::tuple<Ts...>>>                                        // Ts...는 Unique 해야 함
    && !detail::TupleContainsPointers<traits::TupleMap<traits::FlattenTuple<std::tuple<Ts...>>, std::decay_t>>; // Ts...에 포인터 타입이 들어오면 안됨
} // namespace se
