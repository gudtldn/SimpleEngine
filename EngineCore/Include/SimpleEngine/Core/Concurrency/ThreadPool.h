#pragma once
#include <condition_variable>
#include <future>
#include <mutex>
#include <stop_token>
#include <thread>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "tracy/Tracy.hpp"


namespace se::concurrency
{
/**
 *
 * @todo 추후 Work Stealing 방식으로 개선
 */
class SE_CORE_API ThreadPool
{
public:
    explicit ThreadPool(String in_pool_name, uint32 num_threads);
    ~ThreadPool();

    // 이동 & 복사 생성자 제거
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

public:
    template <typename Fn, typename... Args>
    auto Submit(Fn&& func, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>>;

private:
    /** Worker Thread가 실행할 메인 루프 함수 */
    void WorkerLoop(const std::stop_token& token, uint32 thread_id);

private:
    String pool_name;

    TracyLockable(std::mutex, mutex);

#if TRACY_ENABLE
    std::condition_variable_any condition;
#else
    std::condition_variable condition;
#endif

    Array<std::jthread> worker_threads;
    Queue<Function<void()>> tasks;
};

template <typename Fn, typename... Args>
auto ThreadPool::Submit(
    Fn&& func, Args&&... args
) -> std::future<std::invoke_result_t<Fn, Args...>>
{
    using ReturnType = std::invoke_result_t<Fn, Args...>;
    auto task_ptr = std::make_shared<std::packaged_task<ReturnType()>>(
        [func = std::forward<Fn>(func), ...args = std::forward<Args>(args)] mutable -> ReturnType
        {
            return func(std::forward<Args>(args)...);
        }
    );

    {
        std::scoped_lock lock(mutex);
        tasks.Emplace([task_ptr] { (*task_ptr)(); });
    }

    condition.notify_one();
    return task_ptr->get_future();
}
}  // namespace se::concurrency
