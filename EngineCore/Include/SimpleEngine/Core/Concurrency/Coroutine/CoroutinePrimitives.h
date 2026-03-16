#pragma once

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Concurrency/JobHandle.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/JobTask.h"
#include "SimpleEngine/Core/Container/Array.h"

#include <concepts>
#include <coroutine>
#include <type_traits>


namespace se
{
/**
 * 코루틴 실행을 지정된 스레드로 전환합니다.
 *
 * @code
 *   co_await ResumeOn{ EJobThread::Main };   // 메인 스레드로 전환
 *   co_await ResumeOn{ EJobThread::Worker }; // 워커 스레드로 전환
 * @endcode
 */
struct SE_CORE_API ResumeOn
{
    EJobThread target;

    [[nodiscard]] bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle) const;
    void await_resume() const noexcept {}
};

/**
 * 여러 JobHandle이 모두 완료될 때까지 코루틴을 중단합니다.
 * Guard-Count 패턴으로 등록 도중 조기 resume을 방지합니다.
 *
 * @code
 *   auto a = JobSystem::Get().Submit(workA);
 *   auto b = JobSystem::Get().Submit(workB);
 *   co_await WhenAll{ a, b };
 * @endcode
 */
class SE_CORE_API WhenAll
{
public:
    template <typename... Handles>
        requires (sizeof...(Handles) > 0) && (std::convertible_to<Handles, JobHandle> && ...)
    explicit WhenAll(Handles&&... in_handles)
        : WhenAll{ Array<JobHandle>{ std::forward<Handles>(in_handles)...} }
    {
    }

    explicit WhenAll(Array<JobHandle> in_handles);

public:
    [[nodiscard]] bool await_ready() const noexcept;
    bool await_suspend(std::coroutine_handle<> awaiting);
    void await_resume() const noexcept {}

private:
    Array<JobHandle> handles;
};

/**
 * 여러 JobHandle 중 하나라도 완료되면 코루틴을 재개합니다.
 * Atomic Flag 패턴으로 첫 번째 완료만 resume을 트리거합니다.
 *
 * 잔류 콜백의 수명 관리를 위해 원자적 참조 카운팅을 사용합니다.
 * (std::shared_ptr 대신 JobAllocator + 수동 refcount)
 *
 * @code
 *   auto a = JobSystem::Get().Submit(workA);
 *   auto b = JobSystem::Get().Submit(workB);
 *   co_await WhenAny{ a, b };
 * @endcode
 */
class SE_CORE_API WhenAny
{
public:
    template <typename... Handles>
        requires (sizeof...(Handles) > 0) && (std::convertible_to<Handles, JobHandle> && ...)
    explicit WhenAny(Handles&&... in_handles)
        : WhenAny{ Array<JobHandle>{ std::forward<Handles>(in_handles)...} }
    {
    }

    explicit WhenAny(Array<JobHandle> in_handles);

public:
    [[nodiscard]] bool await_ready() const noexcept;
    bool await_suspend(std::coroutine_handle<> awaiting);
    void await_resume() const noexcept {}

private:
    Array<JobHandle> handles;
};


// JobSystem::SubmitTask / DispatchTask 템플릿 정의

template <typename Fn>
    requires std::invocable<Fn>
          && std::same_as<std::invoke_result_t<Fn>, JobTask<void>>
JobHandle JobSystem::SubmitTask(Fn&& factory, EJobPriority priority)
{
    static_assert(
        !std::is_class_v<std::remove_cvref_t<Fn>> || std::is_empty_v<std::remove_cvref_t<Fn>>,
        "[JobSystem Error] DispatchTask requires a stateless (non-capturing) lambda! "
        "Please use a 'static' lambda: []() static -> JobTask<void> { ... }"
    );
    return SubmitTask(std::invoke(std::forward<Fn>(factory)), priority);
}

template <typename Fn>
    requires std::invocable<Fn>
          && std::same_as<std::invoke_result_t<Fn>, JobTask<void>>
void JobSystem::DispatchTask(Fn&& factory, EJobPriority priority)
{
    static_assert(
        !std::is_class_v<std::remove_cvref_t<Fn>> || std::is_empty_v<std::remove_cvref_t<Fn>>,
        "[JobSystem Error] DispatchTask requires a stateless (non-capturing) lambda! "
        "Please use a 'static' lambda: []() static -> JobTask<void> { ... }"
    );
    DispatchTask(std::invoke(std::forward<Fn>(factory)), priority);
}
} // namespace se
