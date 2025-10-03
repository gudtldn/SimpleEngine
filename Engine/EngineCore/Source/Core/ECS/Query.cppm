export module SE.Core:ECS.Query;
import :ECS.World;

import SE.Types;
import SE.Traits;
import SE.Utility;
import std;


namespace se::core::ecs
{
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

namespace details
{
// 타입 T가 FilterTag인지 확인하는 Concept
template <typename T>
concept IsFilterTag =
    traits::type_traits::IsSpecializationOf<T, With>
    || traits::type_traits::IsSpecializationOf<T, Without>;

// 템플릿 파라미터 팩에서 Component 타입만 추출하여 튜플로 변환
template <typename... Ts>
using ExtractFetchTypes = utility::type::TupleCat<
    std::conditional_t<IsFilterTag<Ts>, std::tuple<>, std::tuple<Ts>>...
>;

// 템플릿 파라미터 팩에서 FilterTag만 추출하여 튜플로 변환
template <typename... Ts>
using ExtractedFilterTypes = utility::type::TupleCat<
    std::conditional_t<IsFilterTag<Ts>, std::tuple<Ts>, std::tuple<>>...
>;
}

/**
 * ECS 월드에서 Entity와 Component를 조회하기 위한 Query 인터페이스
 */
export template <typename... Ts>
class Query
{
    using FetchTypes = details::ExtractFetchTypes<Ts...>;     // std::tuple<Comp...>
    using FilterTypes = details::ExtractedFilterTypes<Ts...>; // std::tuple<Filter<Comp...>, ...>

    static_assert(
        traits::type_traits::IsDisjoint<FetchTypes, utility::type::FlattenTuple<FilterTypes>>,
        "Fetch and Filter types must be different"
    );

public:
    explicit Query(World* in_world);
    ~Query() = default;

    Query(const Query&) = default;
    Query& operator=(const Query&) = default;
    Query(Query&&) = default;
    Query& operator=(Query&&) = default;

public:

private:
    World* world;
};

template <typename... Ts>
Query<Ts...>::Query(World* in_world)
    : world(in_world)
{
}
}
