#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/ECS/IStorage.h"
#include "SimpleEngine/ECS/QueryConcepts.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Traits/TupleTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <concepts>
#include <tuple>
#include <type_traits>


namespace se
{
/**
 * 쿼리 파라미터 파싱을 위한 내부 메타프로그래밍 유틸리티
 */
namespace detail
{
// 조건 태그를 사용하여 타입 목록에서 특정 타입들을 추출합니다.
template <template <typename> typename ConditionTag, typename... Ts>
    requires requires { (ConditionTag<Ts>::Value, ...); }
using FilterTypes = traits::TupleCat<
    std::conditional_t<ConditionTag<Ts>::Value, std::tuple<Ts>, std::tuple<>>...
>;

template <template <typename...> typename ConditionTag, typename... Ts>
using FlatFilterTypes = traits::FlattenTuple<FilterTypes<ConditionTag, Ts...>>;

// 가져올 컴포넌트인지 확인 (FilterTag가 아닌 것)
template <typename T>
struct FetchTypePred { static constexpr bool Value = IsFetchTag<T>; };

// 검사할 컴포넌트인지 확인 (필수 컴포넌트)
template <typename T>
struct RequiredComponentPred { static constexpr bool Value = IsRequiredComponent<T>; };

// With<...> 태그인지 확인
template <typename T>
struct WithTagPred { static constexpr bool Value = traits::IsSpecializationOf<T, With>; };

// Without<...> 태그인지 확인
template <typename T>
struct WithoutTagPred { static constexpr bool Value = traits::IsSpecializationOf<T, Without>; };
} // namespace detail

/**
 * 쿼리 파라미터를 분석하고, 엔티티 유효성을 검증하는 로직을 캡슐화한 클래스
 * @tparam Ts 쿼리 파라미터 타입들 (컴포넌트 타입 및 필터 태그)
 */
template <typename... Ts>
class QueryData
{
public:
    // 템플릿 인자들을 분석하여 가져올(Fetch), 포함할(With), 제외할(Without) 타입으로 분류
    using FetchTypes = detail::FilterTypes<detail::FetchTypePred, Ts...>;
    using WithTypes = detail::FlatFilterTypes<detail::WithTagPred, Ts...>;
    using WithoutTypes = detail::FlatFilterTypes<detail::WithoutTagPred, Ts...>;

    // 실제 Query 검증에 사용되는 타입들
    using PredicateTypes = traits::TupleCat<detail::FlatFilterTypes<detail::RequiredComponentPred, Ts...>, WithTypes>;

public:
    explicit QueryData(World* in_world);

    /** 엔티티가 쿼리의 모든 조건(With, Without)을 만족하는지 검증합니다. */
    [[nodiscard]] bool IsEntityValid(Entity entity);

    /** 순회 범위를 최소화하기 위해 가장 작은 컴포넌트 풀(Storage)을 찾습니다. */
    [[nodiscard]] IStorage* FindSmallestPool();

    [[nodiscard]] World* GetWorld() const { return world; }

private:
    World* world;
};

template <typename... Ts>
QueryData<Ts...>::QueryData(World* in_world)
    : world(in_world)
{
}

template <typename... Ts>
bool QueryData<Ts...>::IsEntityValid(Entity entity)
{
    // Fetch(Optional<T> 제외)와 With 목록의 모든 컴포넌트를 가졌는지 확인
    const bool has_all_required =
        traits::ApplyTypes<PredicateTypes>([this, entity]<typename... PredComps>
        {
            return (world->HasComponent<std::decay_t<PredComps>>(entity) && ...);
        });

    if (!has_all_required)
    {
        return false;
    }

    // Without 목록의 컴포넌트를 하나라도 가졌는지 확인
    const bool has_any_excluded =
        traits::ApplyTypes<WithoutTypes>([this, entity]<typename... WithoutComps>
        {
            return (world->HasComponent<std::decay_t<WithoutComps>>(entity) || ...);
        });

    return !has_any_excluded;
}

template <typename... Ts>
IStorage* QueryData<Ts...>::FindSmallestPool()
{
    // 순회의 기준이 될 PredicateTypes(필수 컴포넌트 + With)의 총 개수
    constexpr usize pool_size = std::tuple_size_v<PredicateTypes>;
    if constexpr (pool_size == 0)
    {
        return nullptr;
    }

    // 각 컴포넌트 스토리지 포인터를 배열에 수집
    const auto pools =
        traits::ApplyTypes<PredicateTypes>([this]<typename... PredComps> -> FixedArray<IStorage*, pool_size>
        {
            return {
                world->GetIStorage<std::decay_t<PredComps>>()...
            };
        });

    // 수집된 스토리지 중 가장 크기가 작은 것을 찾아 반환
    return *std::ranges::min_element(pools, [](const IStorage* a, const IStorage* b)
    {
        if (!a) { return false; }
        if (!b) { return true; }

        return a->Len() < b->Len();
    });
}
} // namespace se
