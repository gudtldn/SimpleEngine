#pragma once

#include <tuple>
#include <type_traits>


namespace se::traits
{
template <typename, typename = void>
struct FunctionTraits : std::false_type
{
};

template <typename R, typename... Args>
struct FunctionTraits<R(Args...)>
{
    using Signature = R(Args...);
    using ReturnType = R;
    using ArgumentTypes = std::tuple<Args...>;

    /** 함수의 인자 개수 */
    static constexpr usize Arity = sizeof...(Args);
};

// 함수 포인터
template <typename R, typename... Args>
struct FunctionTraits<R(*)(Args...)> : FunctionTraits<R(Args...)>
{
};

// 멤버 함수 포인터
template <typename R, typename C, typename... Args>
struct FunctionTraits<R(C::*)(Args...)> : FunctionTraits<R(Args...)>
{
};

// const 멤버 함수
template <typename R, typename C, typename... Args>
struct FunctionTraits<R(C::*)(Args...) const> : FunctionTraits<R(Args...)>
{
};

// volatile 멤버 함수
template <typename R, typename C, typename... Args>
struct FunctionTraits<R(C::*)(Args...) volatile> : FunctionTraits<R(Args...)>
{
};

// 람다/함수 객체 지원 (operator() 사용)
template <typename Fn>
struct FunctionTraits<Fn, std::void_t<decltype(&Fn::operator())>> : FunctionTraits<decltype(&Fn::operator())>
{
};
}
