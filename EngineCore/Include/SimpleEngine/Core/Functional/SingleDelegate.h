#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/Function.h"


namespace se
{
template <typename Signature>
class SingleDelegate;

template <typename R, typename... Args>
class SingleDelegate<R(Args...)>
{
    using FunctionType = Function<R(Args...)>;

public:
    SingleDelegate() = default;

public:
    template <typename Fn>
        requires (std::is_invocable_r_v<R, Fn, Args...> && !std::is_member_function_pointer_v<std::decay_t<Fn>>)
    void BindLambda(Fn&& in_lambda)
    {
        callable = std::forward<Fn>(in_lambda);
    }

    void Unbind()
    {
        callable = nullptr;
    }

    [[nodiscard]] bool IsBound() const noexcept
    {
        return callable.IsValid();
    }

    void Execute(Args&&... args) const requires (std::is_void_v<R>)
    {
        if (IsBound())
        {
            return callable(std::forward<Args>(args)...);
        }
    }

    Optional<R> Execute(Args&&... args) const requires (!std::is_void_v<R>)
    {
        if (IsBound())
        {
            return callable(std::forward<Args>(args)...);
        }
        return NullOpt;
    }

private:
    FunctionType callable;
};
}
