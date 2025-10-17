#pragma once

#include <bit>
#include <concepts>
#include <utility>

#define SE_STRINGIFY(x) u8## #x


namespace se::utility
{
/**
 * 특정 값(size)을 지정된 정렬(AlignSize) 크기로 올림(round up)합니다.
 */
template <size_t AlignSize>
    requires (std::has_single_bit(AlignSize))
constexpr size_t AlignedSize(size_t size)
{
    return (size + AlignSize - 1) & ~(AlignSize - 1);
}

/**
 * 특정 타입(T)의 크기를 지정된 정렬(AlignSize) 크기로 올림(round up)합니다.
 */
template <size_t AlignSize, typename T>
    requires (std::has_single_bit(AlignSize))
constexpr size_t AlignedSize()
{
    return AlignedSize<AlignSize>(sizeof(T));
}

/**
 * 주어진 스코프({ ... })의 시작과 끝에서 특정 동작을 자동으로 수행하는 RAII 래퍼
 */
template <typename T>
class ScopeGuard
{
public:
    template <typename... Args>
        requires requires { T::Enter(std::declval<Args>()...); T::Exit(); }
    ScopeGuard(Args&&... args)
    {
        T::Enter(std::forward<Args>(args)...);
    }

    ~ScopeGuard()
    {
        T::Exit();
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(const ScopeGuard&&) = delete;
    ScopeGuard& operator=(const ScopeGuard&&) = delete;
};

/**
 * 스코프를 벗어날 때 주어진 람다(lambda)나 함수 객체를 실행하는 RAII 래퍼
 */
template <typename Fn>
    requires std::invocable<Fn>
class LambdaScopeGuard
{
public:
    LambdaScopeGuard(Fn&& in_exit_func)
        : exit_func(std::forward<Fn>(in_exit_func))
    {
    }

    ~LambdaScopeGuard()
    {
        exit_func();
    }

private:
    Fn exit_func;
};
}
