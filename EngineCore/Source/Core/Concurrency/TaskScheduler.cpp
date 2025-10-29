#include "Core/Concurrency/TaskScheduler.h"

#include <cassert>

#include "Core/Concurrency/ThreadPool.h"
#include "tracy/Tracy.hpp"


namespace se::core::concurrency
{
TaskScheduler* TaskScheduler::Instance = nullptr;

TaskScheduler::TaskScheduler(std::thread::id in_main_thread_id)
    : main_thread_id(in_main_thread_id)
{
    assert(main_thread_id == std::this_thread::get_id() && "TaskScheduler instance is created on a non-main thread!");
    assert(!Instance && "TaskScheduler instance is already created!");

    Instance = this;
}

TaskScheduler::~TaskScheduler()
{
    assert(Instance == this && "TaskScheduler instance is not created!");
    Instance = nullptr;
}

TaskScheduler& TaskScheduler::Get()
{
    assert(Instance && "TaskScheduler instance is not initialized.");
    return *Instance;
}

std::thread::id TaskScheduler::GetMainThreadId() const
{
    return main_thread_id;
}

void TaskScheduler::Launch_MainThread(Task<void>&& task)
{
    if (!task.handle)
    {
        return;
    }

    const auto handle_to_resume = task.handle;

    {
        std::scoped_lock lock(tasks_mutex);

        // Task의 소유권을 이동시겨 수명을 연장
        launched_tasks.Push(std::move(task));
    }

    // Task의 handle을 사용하여 코드를 실행
    handle_to_resume.resume();
}

void TaskScheduler::Launch_WorkerThread(Task<void>&& task)
{
    if (!task.handle)
    {
        return;
    }

    auto handle_to_resume = task.handle;

    {
        std::scoped_lock lock(tasks_mutex);
        // Task의 소유권을 이동시켜 수명을 연장
        launched_tasks.Push(std::move(task));
    }

    // Task의 handle을 사용하여 코드를 실행
    ThreadPool::SubmitTask([handle_to_resume]
    {
        handle_to_resume.resume();
    });
}

void TaskScheduler::ProcessMainThreadTasks()
{
    // 데드락 방지용으로 사용할 변수
    Queue<std::coroutine_handle<>> tasks_to_run;
    {
        std::scoped_lock lock(main_thread_mutex);
        tasks_to_run.Swap(main_thread_tasks);
    }

    // 다시 예약된 모든 코루틴을 재개합니다.
    while (const Optional handle_opt = tasks_to_run.Front())
    {
        tasks_to_run.Pop();
        handle_opt->resume();
    }

    {
        std::scoped_lock lock(tasks_mutex);

        // 실행 완료된 코루틴들을 launched_tasks 에서 제거
        launched_tasks.RemoveIf([](const Task<void>& task)
        {
            return task.handle && task.handle.done();
        });
    }
}

void TaskScheduler::ScheduleOnMainThread(std::coroutine_handle<> handle)
{
    std::scoped_lock lock(main_thread_mutex);
    main_thread_tasks.Push(handle);
}
}
