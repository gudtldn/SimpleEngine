#pragma once
#include <bit>
#include <concepts>
#include <utility>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"

#define SE_CONCAT_NAME_IMPL(a, b) a##b

/** 두 토큰을 하나로 결합합니다. */
#define SE_CONCAT_NAME(a, b) SE_CONCAT_NAME_IMPL(a, b)

/** 중복되지 않는 이름을 생성합니다. */
#define SE_UNIQUE_NAME(name) SE_CONCAT_NAME(name, __COUNTER__)

/** 전달된 인자를 문자열로 변환합니다. */
#define SE_STRINGIFY(x) #x

/** MSVC에서 매크로 확장 문제를 해결하기 위한 매크로 */
#define SE_EXPAND_MACRO(x) x

/**
 * 스코프 종료 시 실행될 코드 블록을 정의합니다. (Defer)
 * @code
 * SE_SCOPE_DEFER {
 *     // ...
 * };
 * @endcode
 */
#define SE_SCOPE_DEFER \
    const LambdaScopeGuard SE_UNIQUE_NAME(_defer_guard_) = [&] -> void

#define SE_SCOPE_DEFER_NAMED(name) \
    LambdaScopeGuard name = [&] -> void

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
    SE_ASSERT(std::has_single_bit(alignment), "Alignment must be a power of two.");
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
class [[nodiscard]] LambdaScopeGuard
{
public:
    LambdaScopeGuard(Fn&& in_exit_func)
        : exit_func(std::forward<Fn>(in_exit_func))
    {
    }

    ~LambdaScopeGuard()
    {
        if (!is_discarded)
        {
            exit_func();
        }
    }

    LambdaScopeGuard(const LambdaScopeGuard&) = delete;
    LambdaScopeGuard& operator=(const LambdaScopeGuard&) = delete;
    LambdaScopeGuard(LambdaScopeGuard&&) noexcept = delete;
    LambdaScopeGuard& operator=(LambdaScopeGuard&&) noexcept = delete;

    /** Defer로 예약된 함수를 취소합니다. */
    void Discard()
    {
        is_discarded = true;
    }

private:
    bool is_discarded = false;
    Fn exit_func;
};

template<typename Fn>
LambdaScopeGuard(Fn) -> LambdaScopeGuard<Fn>;
}  // namespace se
