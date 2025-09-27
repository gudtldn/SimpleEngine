module;
#include "tracy/Tracy.hpp"
export module SE.Core:Concurrency.TaskScheduler;
import :Concurrency.Coroutine;

import SE.Types;
import std;

namespace se::core::engine
{
export class Engine;
}


namespace se::core::concurrency
{
namespace coroutine
{
export struct SwitchToMainThread;
}

/**
 * 비동기 시스템을 관리하는 스케줄러
 */
export class TaskScheduler
{
private:
    // ProcessMainThreadTasks 호출을 위해서
    friend class engine::Engine;

    // ScheduleOnMainThread 호출을 위해서
    friend struct coroutine::SwitchToMainThread;

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
    void Launch_MainThread(coroutine::Task<void>&& task);

    /**
     * 코루틴을 작업 스레드에서 시작합니다. ("Fire-and-forget")
     * @param task 시작할 Task<void> 타입의 코루틴
     */
    void Launch_WorkerThread(coroutine::Task<void>&& task);

    /** Main Thread의 ID를 가져옵니다. */
    std::thread::id GetMainThreadId() const;

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
