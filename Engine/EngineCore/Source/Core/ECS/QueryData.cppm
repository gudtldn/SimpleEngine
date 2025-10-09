export module SE.Core:ECS.QueryData;
import :ECS.World;

import SE.Types;
import SE.Traits;
import SE.Utility;
import std;

#define SE_DEFINE_TYPE_CONDITION_TAG(tag_name, condition) \
template <typename T> \
struct tag_name { static constexpr bool Value = condition; };

using namespace se::traits::type_traits;
using namespace se::utility::type;


namespace se::core::ecs
{
/**
 * 쿼리 파라미터에서 필터링 조건을 명시하는 태그 모음
 */
export inline namespace filters
{
/**
 * 쿼리 결과에 반드시 포함되어야 하는 Component를 지정하는 FilterTag
 * @tparam ComponentTypes 필터링할 Component 타입들
 */
template <typename... ComponentTypes>
struct With
{
    using Types = std::tuple<ComponentTypes...>;
};

/**
 * 쿼리 결과에서 반드시 제외되어야 하는 Component를 지정하는 FilterTag
 * @tparam ComponentTypes 필터링할 Component 타입들
 */
template <typename... ComponentTypes>
struct Without
{
    using Types = std::tuple<ComponentTypes...>;
};
}

/**
 * 쿼리 파라미터 파싱을 위한 내부 메타프로그래밍 유틸리티
 */
namespace details
{
template <typename T, template <typename...> typename... Ts>
concept IsSpecializationTypes = (IsSpecializationOf<T, Ts> || ...);

// 타입 T가 FilterTag인지 확인하는 Concept
template <typename T>
concept IsFetchTag = !IsSpecializationTypes<T, With, Without>;

// 조건 태그(CondFetchTag 등)를 사용하여 타입 목록에서 특정 타입들을 추출
template <template <typename> typename ConditionTag, typename... Ts>
    requires requires { (ConditionTag<Ts>::Value, ...); }
using ExtractTypes = TupleCat<
    std::conditional_t<ConditionTag<Ts>::Value, std::tuple<Ts>, std::tuple<>>...
>;

// 타입 필터링을 위한 조건 태그 정의
SE_DEFINE_TYPE_CONDITION_TAG(CondFetchTag, IsFetchTag<T>);                                           // 가져올 컴포넌트
SE_DEFINE_TYPE_CONDITION_TAG(CondPredicateTag, IsFetchTag<T> && !(IsSpecializationOf<T, Optional>)); // 검사 할 컴포넌트
SE_DEFINE_TYPE_CONDITION_TAG(CondWithTag, (IsSpecializationOf<T, With>));                            // With<...> 태그
SE_DEFINE_TYPE_CONDITION_TAG(CondWithoutTag, (IsSpecializationOf<T, Without>));                      // Without<...> 태그

template <template <typename...> typename ConditionTag, typename... Ts>
using FlattenTypes = FlattenTuple<ExtractTypes<ConditionTag, Ts...>>;
}

/**
 * 쿼리 파라미터를 분석하고, 엔티티 유효성을 검증하는 로직을 캡슐화한 클래스
 * @tparam Ts 쿼리 파라미터 타입들 (컴포넌트 타입 및 필터 태그)
 */
template <typename... Ts>
class QueryData
{
public:
    // 템플릿 인자들을 분석하여 가져올(Fetch), 포함할(With), 제외할(Without) 타입으로 분류
    using FetchTypes = details::ExtractTypes<details::CondFetchTag, Ts...>;
    using WithTypes = details::FlattenTypes<details::CondWithTag, Ts...>;
    using WithoutTypes = details::FlattenTypes<details::CondWithoutTag, Ts...>;

private:
    // 실제 Query 검증에 사용되는 타입들
    using PredicateTypes = TupleCat<details::FlattenTypes<details::CondPredicateTag, Ts...>, WithTypes>;

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
        WithUnpackedTypes<PredicateTypes>([this, entity]<typename... PredComps>
        {
            return (world->HasComponent<std::decay_t<PredComps>>(entity) && ...);
        });

    if (!has_all_required)
    {
        return false;
    }

    // Without 목록의 컴포넌트를 하나라도 가졌는지 확인
    const bool has_any_excluded =
        WithUnpackedTypes<WithoutTypes>([this, entity]<typename... WithoutComps>
        {
            return (world->HasComponent<std::decay_t<WithoutComps>>(entity) || ...);
        });

    if (has_any_excluded)
    {
        return false;
    }

    return true;
}

template <typename... Ts>
IStorage* QueryData<Ts...>::FindSmallestPool()
{
    // 순회의 기준이 될 Fetch + With 컴포넌트 스토리지의 총 개수
    constexpr size_t pool_size =
        std::tuple_size_v<FetchTypes> + std::tuple_size_v<WithTypes>;

    if constexpr (pool_size == 0)
    {
        return nullptr;
    }

    // 각 컴포넌트 스토리지 포인터를 배열에 수집
    std::array<IStorage*, pool_size> pools;
    size_t i = 0;

    WithUnpackedTypes<FetchTypes>([this, &pools, &i]<typename... FetchComps>
    {
        ((pools[i++] = world->GetIStorage<std::decay_t<FetchComps>>()), ...);
    });
    WithUnpackedTypes<WithTypes>([this, &pools, &i]<typename... WithComps>
    {
        ((pools[i++] = world->GetIStorage<std::decay_t<WithComps>>()), ...);
    });

    // 수집된 스토리지 중 가장 크기가 작은 것을 찾아 반환
    return *std::ranges::min_element(pools, [](const IStorage* a, const IStorage* b)
    {
        if (!a) { return false; }
        if (!b) { return true; }

        return a->Length() < b->Length();
    });
}
}
