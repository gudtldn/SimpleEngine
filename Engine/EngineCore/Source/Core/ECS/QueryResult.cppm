export module SimpleEngine.Core:ECS.QueryResult;
import :ECS.Entity;

import SimpleEngine.Types;
import std;


namespace se::core::ecs
{
template <typename... Components>
class QueryResult
{
public:
    QueryResult(Entity in_entity, std::tuple<Components...> in_components)
        : entity(in_entity)
        , components(std::move(in_components))
    {
    }

    // tuple-like interface
    friend Entity& get<0>(QueryResult& qr) { return qr.entity; }

    template <std::size_t I>
    friend decltype(auto) get(QueryResult& qr)
    {
        return std::get<I - 1>(qr.components); // 0은 엔티티, 나머지는 컴포넌트
    }

private:
    Entity entity;
    std::tuple<Components...> components;
};
}

namespace std
{
template <typename... Cs>
struct tuple_size<se::core::ecs::QueryResult<Cs...>> : std::integral_constant<size_t, 1 + sizeof...(Cs)>
{
};

template <size_t I, typename... Cs>
struct tuple_element<I, se::core::ecs::QueryResult<Cs...>>
{
    using type = std::conditional_t<I == 0, se::core::ecs::Entity, std::tuple_element_t<I - 1, std::tuple<Cs...>>>;
};
}
