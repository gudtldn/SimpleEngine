export module SimpleEngine.Traits:FunctionTraits;

import std;


export namespace se::traits::func_traits
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

    static consteval size_t ArgCount() { return sizeof...(Args); }
};

// 함수 포인터
template <typename R, typename... Args>
struct FunctionTraits<R(*)(Args...)> : FunctionTraits<R(Args...)>
{
};

// 멤버 함수 포인터 (일반, const, volatile, ref-qualifier 등)
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
template <typename T>
struct FunctionTraits<T, std::void_t<decltype(&T::operator())>> : FunctionTraits<decltype(&T::operator())>
{
};
}
