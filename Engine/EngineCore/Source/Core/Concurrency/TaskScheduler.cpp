module;
#include "tracy/Tracy.hpp"
module SE.Core;
import :Concurrency.TaskScheduler;


namespace se::core::concurrency
{
TaskScheduler::TaskScheduler(std::thread::id in_main_thread_id)
    : main_thread_id(in_main_thread_id)
{
}

void TaskScheduler::Launch(coroutine::Task<void>&& task)
{
    if (!task.handle)
    {
        return;
    }

    // Task의 소유권을 이동시겨 수명을 연장
    launched_tasks.push_back(std::move(task));

    // Task의 handle을 사용하여 코드를 실행
    launched_tasks.back().handle.resume();
}

void TaskScheduler::ProcessMainThreadTasks()
{
    // 데드락 방지용으로 사용할 변수
    queue<std::coroutine_handle<>> tasks_to_run;
    {
        std::scoped_lock lock(main_thread_mutex);
        tasks_to_run.swap(main_thread_tasks);
    }

    // 다시 예약된 모든 코루틴을 재개합니다.
    while (!tasks_to_run.empty())
    {
        auto handle = tasks_to_run.front();
        tasks_to_run.pop();
        handle.resume();
    }

    // 실행 완료된 코루틴들을 launched_tasks 에서 제거
    std::erase_if(launched_tasks, [](const coroutine::Task<void>& task)
    {
        return task.handle.done();
    });
}

void TaskScheduler::ScheduleOnMainThread(std::coroutine_handle<> handle)
{
    std::scoped_lock lock(main_thread_mutex);
    main_thread_tasks.push(handle);
}
}
