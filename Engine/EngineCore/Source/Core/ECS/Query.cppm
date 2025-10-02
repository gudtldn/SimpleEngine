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

namespace detail
{
// 타입 T가 FilterTag인지 확인하는 Concept
template <typename T>
concept IsFilterTag =
    traits::type_traits::IsSpecializationOf<T, With>
    || traits::type_traits::IsSpecializationOf<T, Without>;

// 템플릿 파라미터 팩에서 Component 타입만 추출하여 튜플로 변환
template <typename... Ts>
using ExtractFetchTypes = decltype(std::tuple_cat(
    std::conditional_t<IsFilterTag<Ts>, std::tuple<>, std::tuple<Ts>>{}...
    // FIXME: Query.cppm(46,70): Error C2512 : 'std::tuple<TransformComponent &>': 사용할 수 있는 적절한 기본 생성자가 없습니다.
));

// 템플릿 파라미터 팩에서 FilterTag만 추출하여 튜플로 변환
template <typename... Args>
using ExtractedFilterTypes = decltype(std::tuple_cat(
    std::conditional_t<IsFilterTag<Args>, std::tuple<Args>, std::tuple<>>{}...
));
}

/**
 * ECS 월드에서 Entity와 Component를 조회하기 위한 Query 인터페이스
 */
export template <typename... Ts>
class Query
{
    using FetchTypes = detail::ExtractFetchTypes<Ts...>;     // std::tuple<Comp...>
    using FilterTypes = detail::ExtractedFilterTypes<Ts...>; // std::tuple<Filter<Comp...>, ...>

public:
    explicit Query(World* in_world);
    ~Query() = default;

    Query(const Query&) = default;
    Query& operator=(const Query&) = default;
    Query(Query&&) = default;
    Query& operator=(Query&&) = default;

public:


private:
    template <typename T>
    T CreateParam();

private:
    World* world;
};

template <typename... Ts>
Query<Ts...>::Query(World* in_world)
    : world(in_world)
{
}

template <typename... Ts>
template <typename T>
T Query<Ts...>::CreateParam()
{
    using namespace se::traits::type_traits;
    if constexpr (IsSpecializationOf<T, With>)
    {
    }
    else if constexpr (IsSpecializationOf<T, Without>)
    {
    }
    else
    {
        static_assert(AlwaysFalse<T>, "Invalid query parameter type");
        std::unreachable();
    }
}
}
