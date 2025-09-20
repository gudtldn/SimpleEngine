export module SE.Core:Concurrency.ThreadPool;
import :Function;

import SE.Types;
import std;


export namespace se::core::concurrency
{
/**
 *
 * @todo 추후 Work Stealing 방식으로 개선
 */
class ThreadPool
{
public:
    explicit ThreadPool(uint32 num_threads = std::thread::hardware_concurrency() * 0.7);
    ~ThreadPool();

    // 이동 & 복사 생성자 제거
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

public:
    void Submit(function::Function<void()> task);

private:
    void WorkerLoop();

private:
    std::atomic<bool> running = false;

    std::mutex mutex;
    std::condition_variable condition;

    vector<std::thread> worker_threads;
    queue<function::Function<void()>> tasks;
};
}
