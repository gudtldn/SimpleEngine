#pragma once

#include "SimpleEngine/ECS/Commands.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/QueryData.h"
#include "SimpleEngine/ECS/Resource.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Traits/FunctionTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <utility>


namespace se::detail
{
template <typename T>
struct SystemParamExtractor
{
    static_assert(traits::AlwaysFalse<T>, "SystemParamExtractor<T> is not specialized for type T");
};

// World& 자체를 요구할 때의 특수화
template <>
struct [[deprecated("use instead Commands")]] SystemParamExtractor<World>
{
    static World& Fetch(World& world)
    {
        return world;
    }
};

// Query<Ts...> 를 요구할 때의 특수화
template <typename... Ts>
struct SystemParamExtractor<Query<Ts...>>
{
    using TargetWorld = QueryData<Ts...>::TargetWorld;

    static Query<Ts...> Fetch(TargetWorld& world)
    {
        return Query<Ts...>{ world };
    }
};

// Commands를 요구할 때의 특수화
template <>
struct SystemParamExtractor<Commands>
{
    static Commands Fetch(World& world)
    {
        CommandBuffer* buffer = world.GetActiveCommandBuffer();
        SE_ASSERT(buffer != nullptr, "Commands can only be used inside a scheduled system.");
        return Commands{ *buffer };
    }
};

// Resource<T>를 요구할 때의 특수화
template <typename T>
struct SystemParamExtractor<Resource<T>>
{
    using RawType = std::remove_cvref_t<T>;
    using TargetWorld = traits::CopyConst<T, World>;

    static Resource<T> Fetch(TargetWorld& world)
    {
        return Resource<T>{ world.template GetResource<RawType>() };
    }
};

/**
 * 일반 함수나 조건식(RunIf)을 ECS에서 사용할 수 있도록 World& 기반 래퍼로 변환합니다.
 * @details 함수의 인자 타입을 분석하여 자동 바인딩하며, 원본 함수의 반환 타입(void, bool 등)을 그대로 유지합니다.
 */
template <typename Fn>
    requires traits::FunctionType<Fn>
auto BindCallable(Fn&& func_obj) -> Function<typename traits::FunctionTraits<std::remove_cvref_t<Fn>>::ReturnType(World&)>
{
    using FnTrait = traits::FunctionTraits<std::remove_cvref_t<Fn>>;
    using ArgsTypes = FnTrait::ArgumentTypes;
    using RetType = FnTrait::ReturnType;

    return [func = std::forward<Fn>(func_obj)](World& world) mutable -> RetType
    {
        return traits::ApplyTypes<ArgsTypes>([&world, &func]<typename... Ts>
        {
            return func(SystemParamExtractor<std::remove_cvref_t<Ts>>::Fetch(world)...);
        });
    };
}
} // namespace se::detail
