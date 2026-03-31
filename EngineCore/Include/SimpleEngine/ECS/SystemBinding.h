#pragma once

#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Traits/FunctionTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <tuple>
#include <utility>


namespace se::detail
{
template <typename T>
struct SystemParamExtractor
{
    static_assert(traits::AlwaysFalse<T>, "SystemParamExtractor<T> is not specialized for type T");
};

// World 포인터 자체를 요구할 때의 특수화
template <>
struct [[deprecated("use instead Query<Commands&, ...>")]] SystemParamExtractor<World*>
{
    static World* Fetch(World* world)
    {
        return world;
    }
};

// Query<Ts...> 를 요구할 때의 특수화
template <typename... Ts>
struct SystemParamExtractor<Query<Ts...>>
{
    static Query<Ts...> Fetch(World* world)
    {
        return Query<Ts...>{ world };
    }
};

/**
 * 일반 함수나 조건식(RunIf)을 ECS에서 사용할 수 있도록 World* 기반 래퍼로 변환합니다.
 * @details 함수의 인자 타입을 분석하여 자동 바인딩하며, 원본 함수의 반환 타입(void, bool 등)을 그대로 유지합니다.
 */
template <typename Fn>
    requires traits::FunctionType<Fn>
auto BindCallable(Fn&& func_obj) -> Function<typename traits::FunctionTraits<std::remove_cvref_t<Fn>>::ReturnType(World*)>
{
    using FnTrait = traits::FunctionTraits<std::remove_cvref_t<Fn>>;
    using RetType = FnTrait::ReturnType;

    return [func = std::forward<Fn>(func_obj)](World* world) mutable -> RetType
    {
        std::tuple tuple = traits::ApplyTypes<typename FnTrait::ArgumentTypes>([world]<typename... Ts>
        {
            return std::make_tuple(SystemParamExtractor<std::remove_cvref_t<Ts>>::Fetch(world)...);
        });
        return std::apply(func, std::move(tuple));
    };
}
} // namespace se::detail
