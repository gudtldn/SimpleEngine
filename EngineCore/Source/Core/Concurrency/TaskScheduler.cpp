// ReSharper disable CppMemberFunctionMayBeConst
#include "Core/Concurrency/TaskScheduler.h"

#include "Core/Concurrency/ThreadPool.h"
#include "Utility/Debug.h"

#include "tracy/Tracy.hpp"


namespace se::concurrency
{
TaskScheduler* TaskScheduler::Instance = nullptr;

TaskScheduler::TaskScheduler(std::thread::id in_main_thread_id)
    : main_thread_id(in_main_thread_id)
{
    SE_ASSERT(main_thread_id == std::this_thread::get_id(), "TaskScheduler instance is created on a non-main thread!");
    SE_ASSERT(!Instance, "TaskScheduler instance is already created!");

    Instance = this;

    // 메인 스레드를 제외한 실제 워커 스레드로 사용 가능한 코어 수
    const uint32 core_count = std::thread::hardware_concurrency();
    const uint32 worker_cores = std::max(1u, core_count - 1);

    // 워커 코어를 Compute(80%) 와 IO(20%)로 분배
    const uint32 compute_threads = std::max(1u, static_cast<uint32>(worker_cores * 0.8));
    const uint32 io_threads = worker_cores - compute_threads;

    compute_pool = std::make_unique<ThreadPool>("Worker Thread", compute_threads);
    io_pool = std::make_unique<ThreadPool>("I/O Thread", io_threads);
}

TaskScheduler::~TaskScheduler()
{
    SE_ASSERT(Instance == this, "TaskScheduler instance is not created!");
    Instance = nullptr;
}

TaskScheduler& TaskScheduler::Get()
{
    SE_ASSERT(Instance, "TaskScheduler instance is not initialized.");
    return *Instance;
}

void TaskScheduler::Launch_MainThread(Task<void>&& task)
{
    if (const auto handle_to_resume = PrepareTaskToLaunch(std::move(task)))
    {
        ScheduleOnMainThread(handle_to_resume);
    }
}

void TaskScheduler::Launch_WorkerThread(Task<void>&& task)
{
    if (const auto handle_to_resume = PrepareTaskToLaunch(std::move(task)))
    {
        ScheduleOnWorkerThread(handle_to_resume);
    }
}

void TaskScheduler::Launch_IOThread(Task<void>&& task)
{
    if (const auto handle_to_resume = PrepareTaskToLaunch(std::move(task)))
    {
        ScheduleOnIOThread(handle_to_resume);
    }
}

std::thread::id TaskScheduler::GetMainThreadId() const
{
    return main_thread_id;
}

Task<void>::HandleType TaskScheduler::PrepareTaskToLaunch(Task<void>&& task)
{
    // 유효하지 않은 handle은 return
    if (!task.handle)
    {
        return {};
    }

    const auto handle_to_resume = task.handle;
    {
        std::scoped_lock lock(tasks_mutex);

        // Task의 소유권을 이동시겨 수명을 연장
        launched_tasks.Push(std::move(task));
    }

    return handle_to_resume;
}

void TaskScheduler::ScheduleOnMainThread(std::coroutine_handle<> handle)
{
    std::scoped_lock lock(main_thread_mutex);
    main_thread_tasks.Push(handle);
}

void TaskScheduler::ScheduleOnWorkerThread(std::coroutine_handle<> handle)
{
    // compute_pool에 작업 제출
    compute_pool->Submit([handle]
    {
        handle.resume();
    });
}

void TaskScheduler::ScheduleOnIOThread(std::coroutine_handle<> handle)
{
    // io_pool에 작업 제출
    io_pool->Submit([handle]
    {
        handle.resume();
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
    while (const Optional handle_opt = tasks_to_run.Pop())
    {
        handle_opt->resume();
    }

    {
        std::scoped_lock lock(tasks_mutex);

        // 실행 완료된 코루틴들을 launched_tasks 에서 제거
        launched_tasks.RemoveIf([](const Task<void>& task)
        {
            return !task.handle || task.handle.done();
        });
    }
}
}
