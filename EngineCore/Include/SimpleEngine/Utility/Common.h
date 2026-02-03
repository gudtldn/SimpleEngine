#pragma once
#include <bit>
#include <concepts>
#include <utility>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"

#define SE_CONCAT_TOKEN_IMPL(a, b) a##b
#define SE_CONCAT_TOKEN(a, b) SE_CONCAT_TOKEN_IMPL(a, b)
#define SE_UNIQUE_TOKEN(name) SE_CONCAT_TOKEN(name, __COUNTER__)
#define SE_STRINGIFY(x) #x

#define SE_SCOPE_DEFER(stmt) \
    const LambdaScopeGuard SE_UNIQUE_TOKEN(defer_guard_){ [&] { stmt; } }


namespace se
{
/**
 * 특정 값(size)을 지정된 정렬(alignment) 크기로 올림(round up)합니다.
 * @param size 원본 크기
 * @param alignment 정렬 단위 (반드시 2의 거듭제곱이어야 함)
 * @return usize 정렬된 크기
 */
constexpr usize AlignedSize(usize size, usize alignment)
{
    SE_ASSERT(std::has_single_bit(alignment));
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * 특정 값(size)을 지정된 정렬(Alignment) 크기로 올림(round up)합니다.
 * @tparam Alignment 정렬 단위 (2의 거듭제곱)
 * @param size 원본 크기
 * @return constexpr usize 정렬된 크기
 */
template <usize Alignment>
    requires (std::has_single_bit(Alignment))
constexpr usize AlignedSize(usize size)
{
    return (size + Alignment - 1) & ~(Alignment - 1);
}

/**
 * 특정 타입(T)의 크기를 지정된 정렬(Alignment) 크기로 올림(round up)합니다.
 * @tparam Alignment 정렬 단위 (2의 거듭제곱)
 * @tparam T 대상 타입
 * @return constexpr usize 정렬된 타입의 크기
 */
template <usize Alignment, typename T>
    requires (std::has_single_bit(Alignment))
constexpr usize AlignedSize()
{
    return AlignedSize<Alignment>(sizeof(T));
}

/**
 * 스코프를 벗어날 때 주어진 람다(lambda)나 함수 객체를 실행하는 RAII 래퍼
 */
template <typename Fn>
    requires std::invocable<Fn&>
class LambdaScopeGuard
{
public:
    explicit LambdaScopeGuard(Fn&& in_exit_func)
        : exit_func(std::forward<Fn>(in_exit_func))
    {
    }

    ~LambdaScopeGuard()
    {
        exit_func();
    }

    // 복사만 금지
    LambdaScopeGuard(const LambdaScopeGuard&) = delete;
    LambdaScopeGuard& operator=(const LambdaScopeGuard&) = delete;
    LambdaScopeGuard(LambdaScopeGuard&&) = default;
    LambdaScopeGuard& operator=(LambdaScopeGuard&&) = default;

private:
    Fn exit_func;
};
}  // namespace se
