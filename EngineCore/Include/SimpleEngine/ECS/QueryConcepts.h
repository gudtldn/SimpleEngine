#pragma once

#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Traits/ContainerTraits.h"
#include "SimpleEngine/Traits/TupleTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <concepts>
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
    static_assert(
        ((std::same_as<Components, std::remove_cvref_t<Components>>) && ...),
        "With<...> requires raw component types. Do not use pointers, references, or const/volatile qualifiers."
    );

    using Types = std::tuple<Components...>;
};

/**
 * 쿼리 결과에서 반드시 제외되어야 하는 Component를 지정하는 FilterTag입니다.
 * @tparam Components 필터링할 Component 타입들
 */
template <typename... Components>
struct Without
{
    static_assert(
        ((std::same_as<Components, std::remove_cvref_t<Components>>) && ...),
        "Without<...> requires raw component types. Do not use pointers, references, or const/volatile qualifiers."
    );

    using Types = std::tuple<Components...>;
};

namespace detail
{
/** 타입 T가 FilterTag(With/Without)가 아닌 Fetch 대상인지 확인합니다. */
template <typename T>
concept IsFetchType = !(traits::IsSpecializationOf<T, With> || traits::IsSpecializationOf<T, Without>);

/** Optional이나 Entity가 아닌 필수 컴포넌트인지 확인합니다. */
template <typename T>
concept IsRequiredComponent = IsFetchType<T> && !(traits::OptionalLike<T> || std::same_as<T, Entity>);

// IsUnique 체크용: With<A, B> 같은 필터 태그를 A, B로 평탄화한 뒤 cvref를 제거합니다.
// Query<A, With<A>> 처럼 직접 명시와 필터 태그 안에서 동시에 등장하는 중복도 감지합니다.
template <typename... Ts>
using ProcessedQueryTuple = traits::TupleMap<
    traits::FlattenTuple<std::tuple<Ts...>>,
    std::remove_cvref_t
>;

/**
 * Query<Ts...> 파라미터의 컴파일 타임 유효성 검사 템플릿
 */
template <typename... Ts>
struct QueryValidator
{
    using ProcessedTypes = ProcessedQueryTuple<Ts...>;

    // 최소 1개 이상의 파라미터가 있어야 함.
    static constexpr bool HasElements = sizeof...(Ts) > 0;

    // 모든 타입이 고유해야 함.
    static constexpr bool IsUnique = traits::TupleUniqueTypes<ProcessedTypes>;

    // 포인터 타입이 없어야 함.
    static constexpr bool NoPointers = !(std::is_pointer_v<std::remove_cvref_t<Ts>> || ...);

    // 모든 타입이 유효한 컴포넌트(class/struct) 이어야 함.
    static constexpr bool AllValidComponents = (std::is_class_v<std::remove_cvref_t<Ts>> && ...);

    // Entity 참조는 불가능 함.
    static constexpr bool NoEntityReference  =
        ((!std::is_reference_v<Ts> || !std::same_as<std::remove_cvref_t<Ts>, Entity>) && ...);

    // Optional 자체의 참조는 불가능 함. (대신 Optional<T& / const T&>를 사용)
    static constexpr bool NoOptionalReference =
        ((!std::is_reference_v<Ts> || !traits::OptionalLike<std::remove_cvref_t<Ts>>) && ...);

    // Optional<Entity>의 형태는 불가능 함.
    static constexpr bool NoOptionalEntity =
        ((!traits::OptionalLike<std::remove_cvref_t<Ts>>
            || !std::same_as<std::remove_cvref_t<traits::InnerOf<std::remove_cvref_t<Ts>>>, Entity>) && ...);
};

/** 단일 쿼리 파라미터가 원본 데이터를 수정하지 않는 읽기 전용 타입인지 확인합니다. */
template <typename T>
struct IsReadOnlyType
{
    using RealType = traits::InnerOf<T>; // Optional<T>이면 내부 타입 T를, 그 외에는 T 그대로 사용
    static constexpr bool Value = !std::is_reference_v<RealType> || std::is_const_v<std::remove_reference_t<RealType>>;
};

/** 쿼리 파라미터 팩 전체가 읽기 전용인지 확인합니다. */
template <typename... Ts>
constexpr bool IS_READ_ONLY_QUERY_PACK = (IsReadOnlyType<Ts>::Value && ...);
} // namespace detail
} // namespace se
