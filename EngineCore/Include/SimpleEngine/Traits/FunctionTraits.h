#pragma once
#include <type_traits>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeList.h"


namespace se::traits
{
template <typename, typename = void>
struct FunctionTraits : std::false_type{};

template <typename R, typename... Args>
struct FunctionTraits<R(Args...)>
{
    using Signature = R(Args...);
    using ReturnType = R;
    using ArgumentTypes = TypeList<Args...>;
};

template <typename R, typename... Args> struct FunctionTraits<R(Args...) noexcept> : FunctionTraits<R(Args...)>{};

// 함수 포인터
template <typename R, typename... Args> struct FunctionTraits<R(*)(Args...)         > : FunctionTraits<R(Args...)>{};
template <typename R, typename... Args> struct FunctionTraits<R(*)(Args...) noexcept> : FunctionTraits<R(Args...)>{};

// 멤버 함수 포인터
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...)                        > : FunctionTraits<R(Args...)>{};
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...)                noexcept> : FunctionTraits<R(Args...)>{};
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...) const                  > : FunctionTraits<R(Args...)>{};
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...) const          noexcept> : FunctionTraits<R(Args...)>{};
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...)       volatile         > : FunctionTraits<R(Args...)>{};
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...)       volatile noexcept> : FunctionTraits<R(Args...)>{};
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...) const volatile         > : FunctionTraits<R(Args...)>{};
template <typename R, typename C, typename... Args> struct FunctionTraits<R(C::*)(Args...) const volatile noexcept> : FunctionTraits<R(Args...)>{};

// 람다/함수 객체 지원 (operator() 사용)
template <typename Fn> struct FunctionTraits<Fn, std::void_t<decltype(&Fn::operator())>> : FunctionTraits<decltype(&Fn::operator())>{};

// 함수, 함수 포인터, 멤버 함수 포인터, 함수 객체처럼 FunctionTraits로 시그니처를 얻을 수 있는 타입
template <typename T>
concept FunctionLike = requires
{
    typename FunctionTraits<std::remove_cvref_t<T>>::Signature;
    typename FunctionTraits<std::remove_cvref_t<T>>::ReturnType;
};
} // namespace se::traits
