export module SE.Core:Concurrency.ThreadPool;
import :Function;

import SE.Types;
import std;


namespace se::core::concurrency
{
/**
 *
 * @todo 추후 Work Stealing 방식으로 개선
 */
export class ThreadPool
{
private:
    static ThreadPool* Instance;

public:
    explicit ThreadPool(uint32 num_threads);
    ~ThreadPool();

    // 이동 & 복사 생성자 제거
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

public:
    template <typename Fn, typename... Args>
    static auto SubmitTask(Fn&& func, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>>;

private:
    template <typename Fn, typename... Args>
    auto Submit(Fn&& func, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>>;

    void WorkerLoop(uint32 thread_id);

private:
    std::atomic<bool> running = false;

    std::mutex mutex;
    std::condition_variable condition;

    vector<std::thread> worker_threads;
    queue<function::Function<void()>> tasks;
};

template <typename Fn, typename... Args>
auto ThreadPool::SubmitTask(
    Fn&& func, Args&&... args
) -> std::future<std::invoke_result_t<Fn, Args...>>
{
    if (!Instance)
    {
        return std::future<std::invoke_result_t<Fn, Args...>>();
    }
    return Instance->Submit(std::forward<Fn>(func), std::forward<Args>(args)...);
}

template <typename Fn, typename... Args>
auto ThreadPool::Submit(
    Fn&& func, Args&&... args
) -> std::future<std::invoke_result_t<Fn, Args...>>
{
    using ReturnType = std::invoke_result_t<Fn, Args...>;
    auto task_ptr = std::make_shared<std::packaged_task<ReturnType()>>(
        [func = std::forward<Fn>(func), ...args = std::forward<Args>(args)] mutable -> ReturnType
        {
            return func(std::move(args)...);
        }
    );

    {
        std::scoped_lock lock(mutex);
        tasks.emplace([task_ptr] { (*task_ptr)(); });
    }

    condition.notify_one();
    return task_ptr->get_future();
}
}
