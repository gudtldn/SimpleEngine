// ReSharper disable CppMemberFunctionMayBeStatic
#pragma once
#include <concepts>
#include <coroutine>
#include <expected>
#include <utility>

#include "SimpleEngine/Core/Concurrency/Coroutine/Task.h"
#include "SimpleEngine/Core/Error/Expected.h"


namespace se::concurrency::detail
{
// C++ Coroutine Promise Trait
/**
 * 모든 Promise 타입들의 공통 로직을 담고 있는 Base 클래스
 * @tparam T 코루틴의 최종 반환 값 타입
 */
template <typename T, typename Derived>
struct PromiseBase
{
private:
    friend Derived;
    PromiseBase() = default;

public:
    /** 코루틴이 처음 생성될 때 호출자에게 반환될 Task 객체를 생성합니다. */
    TaskImpl<T, Derived> get_return_object()
    {
        return TaskImpl<T, Derived>{
            std::coroutine_handle<Derived>::from_promise(static_cast<Derived&>(*this))
        };
    }

    /**
     * 코루틴이 시작된 직후의 동작을 결정합니다.
     * @return `suspend_always`: 코루틴을 즉시 실행하지 않고, 'co_await'되거나 수동 재개될 때까지 대기시킵니다 (Lazy Start).
     */
    std::suspend_always initial_suspend() const noexcept { return {}; }

    /** 코루틴의 본문 실행이 모두 끝난 후(`co_return`, 예외 발생 등)의 동작을 제어하는 Awaiter */
    struct FinalAwaiter
    {
        /** 항상 false를 반환하여 코루틴을 중단시키고 await_suspend를 호출하도록 합니다. */
        bool await_ready() const noexcept
        {
            return false;
        }

        /**
         * 이 코루틴을 기다리고 있던 다른 코루틴(continuation)을 찾아 재개시킵니다.
         * @return 재개할 코루틴의 핸들. 기다리는 코루틴이 없으면 noop_coroutine을 반환
         */
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Derived> continuation_handle) noexcept
        {
            return continuation_handle.promise().continuation
                       ? continuation_handle.promise().continuation
                       : std::noop_coroutine();
        }

        /** 재개 직전에 호출되는 함수 */
        void await_resume() const noexcept
        {
        }
    };

    /** 코루틴이 종료될 때의 동작, 여기서는 FinalAwaiter에 위임 */
    FinalAwaiter final_suspend() noexcept
    {
        return {};
    }

    /** 코루틴 내에서 처리되지 않은 예외가 발생했을 때 호출됩니다. */
    void unhandled_exception()
    {
        result = Unexpected{ std::current_exception() };
    }

public:
    // 나중에 커스텀 메모리 할당자가 필요해질 때 사용
    // void* operator new(usize size);
    // void operator delete(void* ptr, usize size);

public:
    // 이 코루틴의 작업이 끝난 후, 재개되어야 할 다음 코루틴의 핸들
    std::coroutine_handle<> continuation;

    // 코루틴의 최종 결과를 저장
    Expected<T, std::exception_ptr> result;
};

/**
 * T 값을 반환하는 코루틴을 위한 Promise 타입.
 * @tparam T 반환 값의 타입 (void 제외)
 */
template <typename T>
struct Promise : PromiseBase<T, Promise<T>>
{
    template <typename U>
        requires std::convertible_to<U, T>
    void return_value(U&& value)
    {
        this->result.Emplace(std::forward<U>(value));
    }
};

/**
 * void`를 반환하는 코루틴을 위한 Promise 타입 특수화
 */
template <>
struct Promise<void> : PromiseBase<void, Promise<void>>
{
    void return_void()
    {
        this->result.Emplace();
    }
};
}
