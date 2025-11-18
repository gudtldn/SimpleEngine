#pragma once
#include <atomic>
#include <coroutine>
#include <mutex>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "tracy/Tracy.hpp"


namespace se::concurrency
{
/**
 * Task<T>를 MainThread에서 실행합니다.
 */
struct SE_CORE_API SwitchToMainThread
{
    [[nodiscard]] bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle) const;
    void await_resume() const noexcept;
};

/**
 * Task<T>를 WorkerThread에서 실행합니다.
 */
struct SE_CORE_API SwitchToWorkerThread
{
    [[nodiscard]] bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle) const;
    void await_resume() const noexcept;
};

/**
 * Task<T>를 I/O Thread에서 실행합니다.
 */
struct SE_CORE_API SwitchToIOThread
{
    [[nodiscard]] bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle) const;
    void await_resume() const noexcept;
};

class SE_CORE_API EventWaitHandle
{
    struct SE_CORE_API Awaiter
    {
        EventWaitHandle& event;
        std::coroutine_handle<> continuation = nullptr;
        Awaiter* next = nullptr;

        [[nodiscard]] bool await_ready() const noexcept;
        bool await_suspend(std::coroutine_handle<> handle) noexcept;
        void await_resume() const noexcept;
    };

    friend Awaiter;

public:
    /** 이벤트가 Set될 때까지 비동기적으로 대기합니다. */
    Awaiter Wait() noexcept;

    /** 이벤트를 Set하고, 대기 중인 모든 코루틴을 재개합니다. */
    void Set() noexcept;

private:
    TracyLockable(std::mutex, mutex);
    std::atomic<bool> is_set = false;
    Awaiter* waiters_head = nullptr; // Handle 대기열
};
}
