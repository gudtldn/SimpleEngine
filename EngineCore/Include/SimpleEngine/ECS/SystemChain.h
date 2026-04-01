#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/ECS/System.h"

#include <utility>


namespace se
{
// forward declaration
class World;

/**
 * 여러 System을 하나의 그룹으로 묶어 순차적으로 실행하는 클래스
 */
class SE_CORE_API SystemChain
{
public:
    template <typename... Systems>
    explicit SystemChain(Systems&&... systems)
        : systems{ System{ std::forward<Systems>(systems) }... }
    {
    }

public:
    /** Chain 전체의 실행 조건을 추가합니다. */
    template <typename Fn>
        requires traits::FunctionType<Fn>
    SystemChain& RunIf(Fn&& condition)
    {
        preconditions.Push(detail::BindCallable(std::forward<Fn>(condition)));
        return *this;
    }

    /** System을 Chain 순서대로 호출합니다. */
    void Execute(World& world);

private:
    Array<System> systems;
    Array<Function<bool(World&)>> preconditions;
};
} // namespace se
