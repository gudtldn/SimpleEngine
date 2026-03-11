// ReSharper disable CppMemberFunctionMayBeStatic
#pragma once

#include "SimpleEngine/Core/Concurrency/JobAllocator.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>
#include <coroutine>
#include <utility>


namespace se
{
// Forward declaration
template <typename T>
class JobTask;

namespace detail
{
/**
 * co_return 핸들링을 분리하는 Mixin 기저 클래스입니다.
 * C++ 표준상 promise_type에 return_value와 return_void가 동시에
 * 선언되면 컴파일 에러가 발생하므로, 특수화로 분리합니다.
 *
 * @tparam T 코루틴 반환 타입
 * @tparam Derived CRTP 파생 Promise 타입
 */
template <typename T, typename Derived>
struct PromiseReturnMixin
{
    template <typename U>
        requires std::convertible_to<U, T>
    void return_value(U&& value)
    {
        static_cast<Derived*>(this)->result.Emplace(std::forward<U>(value));
    }
};

template <typename Derived>
struct PromiseReturnMixin<void, Derived>
{
    void return_void() noexcept {}
};
} // namespace detail


/**
 * JobTask<T>의 promise_type 구현체
 *
 * 설계 원칙:
 * - Lazy Start: initial_suspend = suspend_always
 * - Symmetric Transfer: await_suspend와 FinalAwaiter 모두 coroutine_handle 반환
 * - Exception-Free: unhandled_exception()에서 SE_ASSERT 크래시
 * - JobAllocator: operator new/delete를 통한 Pool 할당
 *
 * @tparam T 코루틴의 반환 값 타입 (void 가능)
 */
template <typename T>
struct JobTaskPromise : detail::PromiseReturnMixin<T, JobTaskPromise<T>>
{
    // PromiseReturnMixin이 result에 접근해야 합니다.
    friend struct detail::PromiseReturnMixin<T, JobTaskPromise<T>>;

public:
    /** 코루틴 객체(JobTask)를 생성하여 호출자에게 반환합니다. */
    JobTask<T> get_return_object();

    /** co_await 또는 수동 resume까지 실행을 연기합니다. (Lazy Start) */
    std::suspend_always initial_suspend() const noexcept { return {}; }

    /**
     * 코루틴 종료 시 호출자(continuation)로 Symmetric Transfer합니다.
     * continuation이 없으면 noop_coroutine을 반환하여 스택을 정리합니다.
     */
    struct FinalAwaiter
    {
        /** 코루틴을 중단하고, await_suspend에서 호출자(Continuation)로 제어권을 안전하게 전환합니다. */
        bool await_ready() const noexcept { return false; }

        /** 호출자가 있으면 해당 핸들로 전환하고, 없으면 코루틴 실행을 완전히 종료합니다. */
        std::coroutine_handle<> await_suspend(std::coroutine_handle<JobTaskPromise> handle) noexcept
        {
            const JobTaskPromise& promise = handle.promise();
            return promise.continuation ? promise.continuation : std::noop_coroutine();
        }

        /** 재개 직전에 호출되는 함수 */
        void await_resume() const noexcept {}
    };

    /** 코루틴이 종료될 때의 동작, 여기서는 FinalAwaiter에 위임합니다. */
    FinalAwaiter final_suspend() noexcept { return {}; }

    /** 코루틴 내에서 처리되지 않은 예외가 발생했을 때 호출됩니다. */
    void unhandled_exception() noexcept
    {
        SE_FATAL_ERROR(
            "Unhandled exception detected in JobTask. "
            "Exceptions must not propagate through coroutines to ensure engine stability."
        );
    }

    // JobAllocator를 사용하여 메모리 할당
    void* operator new(usize size) { return JobAllocator::Allocate(size); }
    void operator delete(void* ptr, usize) { JobAllocator::Free(ptr); }

public:
    /** 이 코루틴이 완료된 후 재개할 부모 코루틴 핸들 */
    std::coroutine_handle<> continuation;

    /** 코루틴의 반환 값. void 특수화에서는 사용되지 않습니다. */
    Optional<T> result;
};

/** void 특수화 */
template <>
struct JobTaskPromise<void> : detail::PromiseReturnMixin<void, JobTaskPromise<void>>
{
    JobTask<void> get_return_object();

    std::suspend_always initial_suspend() const noexcept { return {}; }

    struct FinalAwaiter
    {
        bool await_ready() const noexcept { return false; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<JobTaskPromise> handle) noexcept
        {
            const JobTaskPromise& promise = handle.promise();
            return promise.continuation ? promise.continuation : std::noop_coroutine();
        }

        void await_resume() const noexcept {}
    };

    FinalAwaiter final_suspend() noexcept { return {}; }

    void unhandled_exception() noexcept
    {
        SE_FATAL_ERROR(
            "Unhandled exception detected in JobTask. "
            "Exceptions must not propagate through coroutines to ensure engine stability."
        );
    }

    void* operator new(usize size) { return JobAllocator::Allocate(size); }
    void operator delete(void* ptr, usize) { JobAllocator::Free(ptr); }

public:
    std::coroutine_handle<> continuation;
};


/**
 * JobSystem과 네이티브하게 통합되는 C++20 코루틴 타입입니다.
 *
 * 핵심 시맨틱스:
 * - `co_await child_task`: **Symmetric Transfer** (Zero-Cost, 스레드 전환 없음)
 * - `co_await ResumeOn{ ... }`: 명시적 스레드 전환 (유일한 스케줄러 경유 경로)
 * - `co_await job_handle`: JobHandle 완료 대기
 *
 * @tparam T 코루틴이 co_return으로 반환하는 값의 타입
 */
template <typename T>
class JobTask
{
public:
    // C++20 coroutine traits
    using promise_type = JobTaskPromise<T>;

    using HandleType = std::coroutine_handle<promise_type>;
    HandleType handle;

public:
    explicit JobTask(HandleType in_handle = nullptr)
        : handle(in_handle)
    {
    }

    ~JobTask()
    {
        if (handle)
        {
            handle.destroy();
        }
    }

    // 코루틴은 복사 불가
    JobTask(const JobTask&) = delete;
    JobTask& operator=(const JobTask&) = delete;

    // 이동 생성자
    JobTask(JobTask&& other) noexcept
        : handle(std::exchange(other.handle, nullptr))
    {
    }

    JobTask& operator=(JobTask&& other) noexcept
    {
        if (this != &other)
        {
            if (handle)
            {
                handle.destroy();
            }
            handle = std::exchange(other.handle, nullptr);
        }
        return *this;
    }

public:
    // C++20 awaitable interface

    /**
     * co_await 시 자식 코루틴이 이미 완료되었는지 확인합니다.
     * 완료된 경우 suspend 없이 즉시 결과를 반환합니다.
     */
    bool await_ready() const noexcept
    {
        return !handle || handle.done();
    }

    /**
     * 부모 코루틴을 중단하고 자식 코루틴으로 Symmetric Transfer합니다.
     * JobSystem 큐잉이 전혀 발생하지 않는 Zero-Cost 경로입니다.
     *
     * @param awaiting_handle 중단되는 부모 코루틴의 핸들
     * @return 즉시 실행할 자식 코루틴의 핸들
     */
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting_handle) noexcept
    {
        handle.promise().continuation = awaiting_handle;
        return handle; // Symmetric Transfer: 자식으로 직접 전환
    }

    /**
     * 코루틴 재개 시 결과를 반환합니다.
     * void 특수화에서는 반환값이 없습니다.
     */
    T await_resume()
    {
        if constexpr (!std::is_void_v<T>)
        {
            SE_ASSERT(handle.promise().result.HasValue(), "JobTask completed without a return value.");
            return std::move(handle.promise().result).Value();
        }
        else
        {
            // void는 그냥 return
            return;
        }
    }
};

template <typename T>
JobTask<T> JobTaskPromise<T>::get_return_object()
{
    return JobTask<T>{ std::coroutine_handle<JobTaskPromise>::from_promise(*this) };
}

inline JobTask<void> JobTaskPromise<void>::get_return_object()
{
    return JobTask{ std::coroutine_handle<JobTaskPromise>::from_promise(*this) };
}
} // namespace se
