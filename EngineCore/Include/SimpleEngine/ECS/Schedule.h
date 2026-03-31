#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/ECS/System.h"
#include "SimpleEngine/ECS/SystemBinding.h"
#include "SimpleEngine/ECS/SystemChain.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <type_traits>
#include <utility>


namespace se
{
/**
 * ECS System의 실행 순서를 관리하고 일괄 실행하는 스케줄러
 */
class SE_CORE_API Schedule
{
public:
    /**
     * 함수, System, SystemChain 등을 스케줄에 추가합니다.
     * @detail 함수 시그니처를 분석하여 필요한 인자를(Query, World* 등)을 자동으로 주입받습니다.
     */
    template <typename... Systems>
    Schedule& Add(Systems&&... systems)
    {
        (AddInternal(std::forward<Systems>(systems)), ...);
        return *this;
    }

    /** Schedule에 등록된 모든 System을 일괄 실행합니다. */
    void Execute(World* world);

private:
    /** 다양한 시스템 타입을 Function<void(World*)> 형태로 type-erased하여 저장합니다. */
    template <typename T>
    void AddInternal(T&& system_obj)
    {
        using DecayedT = std::decay_t<T>;

        // Callable 타입인 경우
        if constexpr (traits::FunctionType<DecayedT>)
        {
            executables.Push(detail::BindCallable(std::forward<T>(system_obj)));
        }

        // System, SystemChain 타입인 경우
        else if constexpr (traits::IsAnyOf<DecayedT, System, SystemChain>)
        {
            executables.Push([obj = std::forward<T>(system_obj)](World* world) mutable -> void
            {
                obj.Execute(world);
            });
        }

        else
        {
            static_assert(traits::AlwaysFalse<DecayedT>, "Invalid system parameter type");
        }
    }

private:
    Array<Function<void(World*)>> executables;
};
} // namespace se
