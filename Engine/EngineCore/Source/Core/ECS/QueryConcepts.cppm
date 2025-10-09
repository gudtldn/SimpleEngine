export module SE.Core:ECS.QueryConcepts;

import SE.Utility;
import SE.Traits;
import std;

using namespace se::utility::type;
using namespace se::traits::type_traits;


namespace se::core::ecs
{
namespace details
{
template <typename TupleType>
struct TupleHasPointerTypesImpl;

template <
    template <typename...> typename TupleLike,
    typename... Ts
>
struct TupleHasPointerTypesImpl<TupleLike<Ts...>>
{
    static constexpr bool Value = (std::is_pointer_v<Ts> || ...);
};

template <typename TupleType>
concept TupleHasPointerTypes = TupleHasPointerTypesImpl<TupleType>::Value;
}

export template <typename... Ts>
concept QueryParameterPack =
    sizeof...(Ts) > 0                                                                           // Ts...의 개수는 1개 이상
    && TupleHasUniqueTypes<FlattenTuple<std::tuple<Ts...>>>                                     // Ts...는 Unique 해야 함
    && !details::TupleHasPointerTypes<TupleMap<FlattenTuple<std::tuple<Ts...>>, std::decay_t>>; // Ts...에 포인터 타입이 들어오면 안됨
}
