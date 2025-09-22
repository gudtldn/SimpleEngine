module;
#include "tracy/Tracy.hpp"
export module SE.Core:Concurrency.TaskScheduler;
import :Concurrency.Coroutine;

import SE.Types;
import std;

namespace se::core::engine
{
class Engine;
}


namespace se::core::concurrency
{
/**
 * 비동기 시스템을 관리하는 스케줄러
 */
class TaskScheduler
{
    friend class engine::Engine;

    static TaskScheduler* Instance;
    explicit TaskScheduler(std::thread::id in_main_thread_id);

public:
    ~TaskScheduler();

    // 이동 & 복사 생성자 제거
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;

public:
    static void LaunchTask(coroutine::Task<void>&& task);

    static std::thread::id GetMainThreadId();

private:
    /**
     * 코루틴을 시작합니다. ("Fire-and-forget")
     * @param task 시작할 Task<void> 타입의 코루틴
     */
    void Launch(coroutine::Task<void>&& task);

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

    // 메인 스레드에서 실행되기를 기다리는 코루틴 핸들 큐
    queue<std::coroutine_handle<>> main_thread_tasks;

    // Launch로 시작된 최상위 코루틴들의 생명 주기를 관리하는 벡터
    // Task 객체가 파괴되면 코루틴 상태도 파괴되므로, 끝날 때까지 보관
    vector<coroutine::Task<void>> launched_tasks;
};
}
