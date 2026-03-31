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

template <typename... Ts>
struct QueryValidator;

template <
    template <typename...> typename TupleLike,
    typename... ProcessedTs
>
struct QueryValidator<TupleLike<ProcessedTs...>>
{
    // 최소 1개 이상의 파라미터가 있는지
    static constexpr bool HasElements = sizeof...(ProcessedTs) > 0;

    // 모든 타입이 고유한지
    static constexpr bool IsUnique = traits::TupleUniqueTypes<TupleLike<ProcessedTs...>>;

    // 포인터 타입이 없는지
    static constexpr bool NoPointers = !(std::is_pointer_v<ProcessedTs> || ...);

    // 모든 타입이 유효한 컴포넌트(class/struct)인지
    static constexpr bool AllValidComponents = (std::is_class_v<ProcessedTs> && ...);

    // 최종 결과
    static constexpr bool IsValidPack = HasElements && IsUnique && NoPointers && AllValidComponents;
};

template <typename... Ts>
consteval bool ValidateQueryPack()
{
    using RawTuple = std::tuple<Ts...>;
    using FlattenedTuple = traits::FlattenTuple<RawTuple>;
    using ProcessedTuple = traits::TupleMap<FlattenedTuple, std::decay_t>;

    return QueryValidator<ProcessedTuple>::IsValidPack;
}
} // namespace detail

template <typename... Ts>
concept QueryParameterPack = detail::ValidateQueryPack();
} // namespace se
