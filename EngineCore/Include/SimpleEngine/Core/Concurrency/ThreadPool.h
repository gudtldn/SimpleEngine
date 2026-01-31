#pragma once
#include <condition_variable>
#include <future>
#include <mutex>
#include <stop_token>
#include <thread>
#include <functional>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "tracy/Tracy.hpp"


namespace se
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
    std::condition_variable_any condition;

    Array<std::jthread> worker_threads;
    Queue<std::move_only_function<void()>> tasks;
};

template <typename Fn, typename... Args>
auto ThreadPool::Submit(Fn&& func, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>>
{
    using ReturnType = std::invoke_result_t<Fn, Args...>;
    std::packaged_task<ReturnType()> task{
        std::bind_front(std::forward<Fn>(func), std::forward<Args>(args)...)
    };

    std::future<ReturnType> result_future = task.get_future();
    {
        std::scoped_lock lock(mutex);
        tasks.Emplace([task = std::move(task)]() mutable
        {
            task();
        });
    }

    condition.notify_one();
    return result_future;
}
}  // namespace se
