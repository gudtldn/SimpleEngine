// ReSharper disable CppMemberFunctionMayBeStatic
#include "Core/Concurrency/Coroutine/Awaitables.h"

#include "Core/Concurrency/TaskScheduler.h"
#include "Core/Concurrency/ThreadPool.h"


namespace se::core::concurrency::coroutine
{
bool SwitchToWorkerThread::await_ready() const noexcept
{
    // SwitchTo...은 항상 스레드를 전환해야 하므로, false를 반환
    return false;
}

void SwitchToWorkerThread::await_suspend(std::coroutine_handle<> handle) const
{
    // 현재 코루틴을 중단하고, ThreadPool에서 다시 resume()을 호출
    ThreadPool::SubmitTask([handle]
    {
        handle.resume();
    });
}

void SwitchToWorkerThread::await_resume() const noexcept
{
}

bool SwitchToMainThread::await_ready() const noexcept
{
    // 이미 메인 스레드면 그냥 넘어가기
    return std::this_thread::get_id() == TaskScheduler::Get().GetMainThreadId();
}

void SwitchToMainThread::await_suspend(std::coroutine_handle<> handle) const
{
    // 현재 작업 스레드를 메인스레드로 변경
    TaskScheduler::Get().ScheduleOnMainThread(handle);
}

void SwitchToMainThread::await_resume() const noexcept
{
}

bool EventWaitHandle::Awaiter::await_ready() const noexcept
{
    return event.is_set.load(std::memory_order_acquire);
}

bool EventWaitHandle::Awaiter::await_suspend(std::coroutine_handle<> handle) noexcept
{
    continuation = handle;
    {
        std::scoped_lock lock(event.mutex);

        if (event.is_set.load(std::memory_order_relaxed))
        {
            // suspend 하려는 순간 다른 스레드가 Set 해버린 경우
            return false;
        }

        // 대기열에 추가
        this->next = event.waiters_head;
        event.waiters_head = this;
    }
    return true;
}

void EventWaitHandle::Awaiter::await_resume() const noexcept
{
}

EventWaitHandle::Awaiter EventWaitHandle::Wait() noexcept
{
    return { *this };
}

void EventWaitHandle::Set() noexcept
{
    if (is_set.exchange(true, std::memory_order_acq_rel))
    {
        return;
    }

    const Awaiter* waiters_to_resume;
    {
        std::scoped_lock lock(mutex);
        waiters_to_resume = std::exchange(waiters_head, nullptr);
    }

    // 모든 대기열 재개
    while (waiters_to_resume)
    {
        const Awaiter* next = waiters_to_resume->next;
        waiters_to_resume->continuation.resume();
        waiters_to_resume = next;
    }
}
}
