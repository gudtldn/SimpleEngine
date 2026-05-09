#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/ECS/QueryConcepts.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Traits/TupleTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <algorithm>
#include <memory>
#include <tuple>
#include <type_traits>


namespace se::detail
{
/**
 * 쿼리 파라미터 파싱을 위한 내부 메타프로그래밍 유틸리티
 */

// Ts...에서 ConditionTag<T>::Value가 true인 타입들만 추출합니다.
// 예) FilterTypes<FetchTypePred, A, B, With<C>> -> std::tuple<A, B, With<C>>
template <template <typename> typename ConditionTag, typename... Ts>
using FilterTypes = traits::TupleCat<
    std::conditional_t<ConditionTag<Ts>::Value, std::tuple<Ts>, std::tuple<>>...
>;

// FilterTypes 결과를 평탄화합니다. With<A,B> 같은 필터 태그를 A, B로 전개할 때 사용합니다.
// 예) FlatFilterTypes<WithTagPred, A, With<B, C>> -> std::tuple<B, C>
template <template <typename...> typename ConditionTag, typename... Ts>
using FlatFilterTypes = traits::FlattenTuple<FilterTypes<ConditionTag, Ts...>>;

// 가져올 컴포넌트인지 확인 (FilterTag가 아닌 것)
template <typename T>
struct FetchTypePred { static constexpr bool Value = IsFetchType<T>; };

// 검사할 컴포넌트인지 확인 (필수 컴포넌트)
template <typename T>
struct RequiredComponentPred { static constexpr bool Value = IsRequiredComponent<T>; };

// With<...> 태그인지 확인
template <typename T>
struct WithTagPred { static constexpr bool Value = traits::IsSpecializationOf<T, With>; };

// Without<...> 태그인지 확인
template <typename T>
struct WithoutTagPred { static constexpr bool Value = traits::IsSpecializationOf<T, Without>; };


/**
 * 쿼리 파라미터를 분석하고, Entity 유효성을 검증하는 로직을 캡슐화한 클래스
 * @tparam Ts 쿼리 파라미터 타입들 (컴포넌트 타입 및 필터 태그)
 */
template <typename... Ts>
class QueryData
{
public:
    // Query가 읽기 전용인지 확인
    static constexpr bool IsReadOnly = IsReadOnlyQueryPack<Ts...>;
    using TargetWorld = std::conditional_t<IsReadOnly, const World, World>;

    // 템플릿 인자들을 분석하여 가져올(Fetch), 포함할(With), 제외할(Without) 타입으로 분류
    using FetchTypes = FilterTypes<FetchTypePred, Ts...>;
    using WithTypes = FlatFilterTypes<WithTagPred, Ts...>;
    using WithoutTypes = FlatFilterTypes<WithoutTagPred, Ts...>;

    // 실제 Query 검증에 사용되는 타입들(Fetch(Optional, Entity 제외) + With)
    using PredicateTypes = traits::TupleCat<FlatFilterTypes<RequiredComponentPred, Ts...>, WithTypes>;

    static constexpr usize NumPredicates = std::tuple_size_v<PredicateTypes>;
    static constexpr usize NumWithout = std::tuple_size_v<WithoutTypes>;

public:
    explicit QueryData(TargetWorld& in_world)
        : world(std::addressof(in_world))
    {
        // Predicate Storage 캐싱
        if constexpr (NumPredicates > 0)
        {
            predicate_pools = traits::ApplyTypes<PredicateTypes>([this]<typename... PredComps> -> decltype(predicate_pools)
            {
                return { world->template FindRawStorage<std::remove_cvref_t<PredComps>>()... };
            });

            // 아직 만들어지지 않은 Storage가 1개라도 있는 경우 -> 조건에 부합하지 않음
            is_valid_query = std::ranges::all_of(predicate_pools, [](const IComponentStorage* pool) { return pool != nullptr; });
        }

        // Without Storage 캐싱
        if constexpr (NumWithout > 0)
        {
            auto temp_without_pools = traits::ApplyTypes<WithoutTypes>([this]<typename... WithoutComps> -> decltype(without_pools)
            {
                return { world->template FindRawStorage<std::remove_cvref_t<WithoutComps>>()... };
            });

            for (const IComponentStorage* pool : temp_without_pools)
            {
                if (pool != nullptr)
                {
                    without_pools[valid_without_count++] = pool;
                }
            }
        }
    }

    /** Entity가 쿼리의 모든 조건(With, Without)을 만족하는지 검증합니다. */
    [[nodiscard]] bool IsEntityValid(Entity entity) const
    {
        // 쿼리 자체가 유효하지 않으면 return
        if (!is_valid_query)
        {
            return false;
        }

        // 필수 포함 조건 확인 (Fetch(Optional<T> 제외)와 With 목록)
        // 모든 ComponentPool에 Entity가 존재해야 함
        for (const IComponentStorage* pool : predicate_pools)
        {
            // 필수 조건에 부합하지 않으면 return
            if (!pool->Contains(entity))
            {
                return false;
            }
        }

        if constexpr (NumWithout > 0)
        {
            // 제외 조건 확인 (Without 목록)
            // ComponentPool에 Entity가 하나라도 존재하면 안 됨
            for (usize i = 0; i < valid_without_count; ++i)
            {
                // 제외 조건에 있을경우 return
                if (without_pools[i]->Contains(entity))
                {
                    return false;
                }
            }
        }

        return true;
    }

    /** 순회 범위를 최소화하기 위해 가장 작은 ComponentPool을 찾습니다. */
    [[nodiscard]] const IComponentStorage* FindSmallestPool() const
    {
        if constexpr (NumPredicates == 0)
        {
            return nullptr;
        }

        if (!is_valid_query)
        {
            return nullptr;
        }

        // 필수 ComponentPool 중 가장 크기가 작은 것을 찾아 반환
        return *std::ranges::min_element(predicate_pools, [](const IComponentStorage* a, const IComponentStorage* b)
        {
            return a->Len() < b->Len();
        });
    }

    [[nodiscard]] TargetWorld& GetWorld() const { return *world; }

private:
    TargetWorld* world;

    // 매번 HashMap 조회를 피하기 위한 IComponentStorage* 배열
    FixedArray<const IComponentStorage*, NumPredicates> predicate_pools{};
    FixedArray<const IComponentStorage*, NumWithout> without_pools{};
    usize valid_without_count = 0;
    // 쿼리가 유효한지 검증하는 flag
    bool is_valid_query = true;
};
} // namespace se::detail
