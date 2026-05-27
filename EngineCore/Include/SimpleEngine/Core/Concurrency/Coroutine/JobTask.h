// ReSharper disable CppMemberFunctionMayBeStatic
#pragma once

#include "SimpleEngine/Core/Concurrency/JobAllocator.h"
#include "SimpleEngine/Core/Concurrency/JobHandle.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>
#include <coroutine>
#include <memory>
#include <utility>


namespace se
{
// Forward declarations
template <typename T>
class JobTask;

namespace detail
{
template <typename T>
struct JobSharedState;

struct EmptySharedState{};

/**
 * co_return 핸들링을 분리하는 Mixin 클래스
 * C++ 표준상 promise_type에 return_value와 return_void가 동시에
 * 선언되면 컴파일 에러가 발생하므로, 특수화로 분리합니다.
 *
 * @tparam T 코루틴 반환 타입
 * @tparam Derived CRTP 파생 Promise 타입
 */
template <typename T, typename Derived>
class PromiseReturnMixin
{
    friend Derived;
    PromiseReturnMixin() = default;

public:
    void return_value(T value)
    {
        static_cast<Derived*>(this)->storage.Emplace(std::move(value));
    }
};

template <typename Derived>
class PromiseReturnMixin<void, Derived>
{
    friend Derived;
    PromiseReturnMixin() = default;

public:
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
    // T가 void일 경우, EmptyType + [[no_unique_address]]를 사용하여 메모리 최적화
    using StorageType = std::conditional_t<std::is_void_v<T>, EmptyType, Optional<T>>;
    using SharedStateType = std::conditional_t<std::is_void_v<T>, detail::EmptySharedState, std::shared_ptr<detail::JobSharedState<T>>>;

public:
    /** 일반적인 코루틴, static 람다, 일반 함수용 생성자 */
    JobTaskPromise() = default;

    /** non-static 람다(캡처가 있는 람다)용 생성자 */
    template <typename ClassType, typename... Args>
        requires std::invocable<ClassType, Args&&...>
              && std::same_as<std::invoke_result_t<ClassType, Args&&...>, JobTask<T>>
              && std::is_class_v<std::remove_cvref_t<ClassType>>
    JobTaskPromise(ClassType&&, Args&&...)
    {
        static_assert(
            traits::AlwaysFalse<ClassType, Args...>,
            "[JobTask Error] Stateful (capturing) lambdas are not allowed in JobTask coroutines to prevent dangling references! "
            "Please use a 'static' lambda and pass required data via parameters. "
            "(e.g., [](...) static -> JobTask<T> { ... })"
        );
    }

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

        /**
         * 호출자가 있으면 해당 핸들로 전환하고, 없으면 코루틴 실행을 완전히 종료합니다.
         * Detached 모드(SubmitTask/DispatchTask로 제출된 코루틴)일 경우, 프레임을 자동 소멸하고 완료 카운터를 감소시킵니다.
         */
        std::coroutine_handle<> await_suspend(std::coroutine_handle<JobTaskPromise> handle) noexcept
        {
            JobTaskPromise& promise = handle.promise();

            // 부모 코루틴이 있다면 복귀 (Symmetric Transfer)
            if (promise.continuation)
            {
                return promise.continuation;
            }

            // Detached 모드인 경우 (코루틴 소멸 + 카운터 통지)
            if (promise.detached)
            {
                // 프레임 소멸 전에 카운터 및 값 추출 (destroy시 promise도 같이 사라지기 때문)
                auto counter = std::move(promise.completion_counter);

                if constexpr (!std::is_void_v<T>)
                {
                    auto state = std::move(promise.shared_state);
                    if (state && promise.storage.HasValue())
                    {
                        state->result.Emplace(std::move(promise.storage).Value());
                    }
                }

                // 코루틴 프레임 파괴 (여기서 promise도 날아감)
                handle.destroy();

                // Waiter에게 완료 통지
                if (counter)
                {
                    counter->Decrement();
                }
            }

            // 코루틴 정상 종료
            return std::noop_coroutine();
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

    /**
     * Detached 모드 플래그
     * true일 경우 FinalAwaiter에서 코루틴 프레임을 자동 소멸합니다.
     * SubmitTask() / DispatchTask()에 의해 설정됩니다.
     */
    bool detached = false;

    /**
     * Detached 모드에서 완료 시 Decrement할 카운터
     * SubmitTask()에 의해 설정됩니다. DispatchTask()에서는 null입니다.
     */
    std::shared_ptr<JobCounter> completion_counter;

    /** 외부로 값을 전달할 공유 상태 포인터 */
    NO_UNIQUE_ADDRESS SharedStateType shared_state;

    /** 코루틴의 반환 값 */
    NO_UNIQUE_ADDRESS StorageType storage;
};

/**
 * 엔진에서 사용하는 기본적인 C++20 코루틴 타입
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
        if constexpr (std::is_void_v<T>)
        {
            // void는 그냥 return
            return;
        }
        else
        {
            SE_ASSERT(handle.promise().storage.HasValue(), "JobTask completed without a return value.");
            return std::move(handle.promise().storage).Value();
        }
    }
};

template <typename T>
JobTask<T> JobTaskPromise<T>::get_return_object()
{
    return JobTask<T>{ std::coroutine_handle<JobTaskPromise>::from_promise(*this) };
}
} // namespace se
