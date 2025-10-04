export module SE.Core:ECS.Query;
import :ECS.World;

import SE.Types;
import SE.Traits;
import SE.Utility;
import std;

#define SE_DEFINE_TYPE_CONDITION_TAG(tag_name, condition) \
template <typename T> \
struct tag_name { static constexpr bool Value = condition; }

using namespace se::traits::type_traits;
using namespace se::utility::type;

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
template <typename T, template <typename...> typename... Ts>
concept IsSpecializationTypes = (IsSpecializationOf<T, Ts> || ...);

// 타입 T가 FilterTag인지 확인하는 Concept
template <typename T>
concept IsFilterTag = IsSpecializationTypes<T, With, Without>;

template <template <typename> typename ConditionType, typename... Ts>
    requires requires { { (ConditionType<Ts>::Value, ...) } -> std::same_as<bool>; }
using ExtractTypes = TupleCat<
    std::conditional_t<ConditionType<Ts>::Value, std::tuple<Ts>, std::tuple<>>...
>;

SE_DEFINE_TYPE_CONDITION_TAG(CondFetchTag, !IsFilterTag<T>);
SE_DEFINE_TYPE_CONDITION_TAG(CondFilterTag, IsFilterTag<T>);
SE_DEFINE_TYPE_CONDITION_TAG(CondWithTag, (IsSpecializationOf<T, With>));
SE_DEFINE_TYPE_CONDITION_TAG(CondWithoutTag, (IsSpecializationOf<T, Without>));

// 템플릿 파라미터 팩에서 Component 타입만 추출하여 튜플로 변환
template <typename... Ts>
using ExtractFetchTypes = ExtractTypes<CondFetchTag, Ts...>;

// 템플릿 파라미터 팩에서 FilterTag만 추출하여 튜플로 변환
template <typename... Ts>
using ExtractedFilterTypes = ExtractTypes<CondFilterTag, Ts...>;

// With<Ts...>의 Ts 타입들만 추출
template <typename... Ts>
using ExtractedWithTypes = FlattenTuple<ExtractTypes<CondWithTag, Ts...>>;

// Without<Ts...>의 Ts 타입들만 추출
template <typename... Ts>
using ExtractedWithoutTypes = FlattenTuple<ExtractTypes<CondWithoutTag, Ts...>>;
}

/**
 * ECS 월드에서 Entity와 Component를 조회하기 위한 Query 인터페이스
 */
export template <typename... Ts>
    requires (sizeof...(Ts) != 0)
class Query
{
    using FetchTypes = details::ExtractFetchTypes<Ts...>;     // std::tuple<Comp...>
    using FilterTypes = details::ExtractedFilterTypes<Ts...>; // std::tuple<Filter<Comp...>, ...>

    // FetchTypes와 FilterTypes의 컴포넌트 타입이 겹치면 안됨
    static_assert(
        IsDisjoint<FetchTypes, FlattenTuple<FilterTypes>>,
        "A component cannot be both fetched and used in a filter (With/Without)."
    );

    // With와 Without의 컴포넌트 타입이 겹치면 안됨
    static_assert(
        IsDisjoint<details::ExtractedWithTypes<Ts...>, details::ExtractedWithoutTypes<Ts...>>,
        "A component cannot be present in both With<> and Without<> filters."
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
    requires (sizeof...(Ts) != 0)
Query<Ts...>::Query(World* in_world)
    : world(in_world)
{
}
}
