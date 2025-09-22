module SE.Core;
import :Concurrency.Coroutine.Awaitables;

import :Concurrency.ThreadPool;


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
}
