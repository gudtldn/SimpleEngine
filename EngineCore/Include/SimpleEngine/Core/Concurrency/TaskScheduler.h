#pragma once
#include <coroutine>
#include <future>
#include <mutex>
#include <thread>

#include "Coroutine.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/Task.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "tracy/Tracy.hpp"

namespace se::core
{
class Engine;
}


namespace se::core::concurrency
{
namespace coroutine
{
struct SwitchToMainThread;
}

struct TaskSchedulerTest;


/**
 * 비동기 시스템을 관리하는 스케줄러
 */
class SE_CORE_API TaskScheduler
{
private:
    // ProcessMainThreadTasks 호출을 위해서
    friend class se::core::Engine;

    // ScheduleOnMainThread 호출을 위해서
    friend struct coroutine::SwitchToMainThread;

    // 코드 테스트를 위해서
    friend struct TaskSchedulerTest;

    static TaskScheduler* Instance;

public:
    explicit TaskScheduler(std::thread::id in_main_thread_id);
    ~TaskScheduler();

    // 이동 & 복사 생성자 제거
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;

public:
    /** TaskScheduler의 인스턴스를 가져옵니다. */
    static TaskScheduler& Get();

    /**
     * 코루틴을 메인 스레드에서 시작합니다. ("Fire-and-forget")
     * @param task 시작할 Task<void> 타입의 코루틴
     */
    void Launch_MainThread(Task<void>&& task);

    /**
     * 코루틴을 작업 스레드에서 시작합니다. ("Fire-and-forget")
     * @param task 시작할 Task<void> 타입의 코루틴
     */
    void Launch_WorkerThread(Task<void>&& task);

    /**
     * Task가 완료될 때까지 현재 스레드를 블로킹하고 결과를 반환합니다.
     * @warning ThreadPool이 관리하고 있는 Thread에서 호출하면 데드락이 발생할 수도 있습니다.
     * @param task 기다릴 Task 객체
     * @return Task의 결과값
     */
    template <typename T>
    T BlockOn(Task<T>&& task);

    /** Main Thread의 ID를 가져옵니다. */
    [[nodiscard]] std::thread::id GetMainThreadId() const;

private:
    /**
     * Main Loop에서 매 프레임 호출되어야 합니다.
     * 예약된 메인 스레드 작업들을 실행하고, 완료된 최상위 코루틴들을 정리합니다.
     */
    void ProcessMainThreadTasks();

    /**
     * 워커 스레드나 I/O 스레드에서 메인 스레드로 코루틴의 실행을 예약합니다.
     * @param handle 메인 스레드에서 재개될 코루틴의 핸들
     */
    void ScheduleOnMainThread(std::coroutine_handle<> handle);

private:
    std::thread::id main_thread_id;
    TracyLockable(std::mutex, main_thread_mutex);
    TracyLockable(std::mutex, tasks_mutex);

    // 메인 스레드에서 실행되기를 기다리는 코루틴 핸들 큐
    queue<std::coroutine_handle<>> main_thread_tasks;

    // Launch로 시작된 최상위 코루틴들의 생명 주기를 관리하는 벡터
    // Task 객체가 파괴되면 코루틴 상태도 파괴되므로, 끝날 때까지 보관
    Array<Task<void>> launched_tasks;
};

template <typename T>
T TaskScheduler::BlockOn(Task<T>&& task)
{
    if (!task.handle)
    {
        throw std::runtime_error("Cannot BlockOn an invalid Task.");
    }

    if (task.await_ready())
    {
        return task.await_resume();
    }

    std::promise<T> promise;
    auto future = promise.get_future();

    Launch_WorkerThread([](Task<T> t, std::promise<T> p) -> Task<void>
    {
        try
        {
            T result = co_await t;
            p.set_value(std::move(result));
        }
        catch (...)
        {
            p.set_exception(std::current_exception());
        }
    }(std::move(task), std::move(promise)));

    return future.get();
}


// TODO: 전처리기로 테스트일때만 컴파일
struct TaskSchedulerTest
{
    TaskSchedulerTest(TaskScheduler& in_scheduler)
        : scheduler(in_scheduler)
    {
    }

    void ProcessMainThreadTasks() const
    {
        scheduler.ProcessMainThreadTasks();
    }

    TaskScheduler& scheduler;
};
}
