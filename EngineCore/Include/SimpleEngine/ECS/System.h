#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/ECS/SystemBinding.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <utility>


namespace se
{
// forward declaration
class World;

/**
 * ECS에서 사용하는 단일 System 클래스
 */
class System
{
public:
    template <typename Fn>
        requires traits::FunctionType<Fn>
    explicit System(Fn&& in_system)
        : system(detail::BindCallable(std::forward<Fn>(in_system)))
    {
    }

public:
    /** System의 실행 조건을 추가합니다. */
    template <typename Fn>
        requires traits::FunctionType<Fn>
    System& RunIf(Fn&& condition)
    {
        preconditions.Push(detail::BindCallable(std::forward<Fn>(condition)));
        return *this;
    }

    /** 모든 조건을 검사한 후 System을 실행합니다. */
    void Execute(World* world);

private:
    Function<void(World*)> system;
    Array<Function<bool(World*)>> preconditions;
};
} // namespace se
