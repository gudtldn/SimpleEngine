module;
#include <cassert>
module SE.Core;
import :Concurrency.ThreadPool;

import SE.Utility;
import SE.Platform;

using namespace se::core::function;


namespace se::core::concurrency
{
ThreadPool* ThreadPool::Instance = nullptr;

ThreadPool::ThreadPool(uint32 num_threads)
    : worker_threads(num_threads)
{
    assert(!Instance && "ThreadPool instance is already created!");
    Instance = this;

    // Worker Thread 생성
    running = true;
    for (auto [n, thread] : worker_threads | std::views::enumerate)
    {
        thread = std::thread{
            &ThreadPool::WorkerLoop, this,
            static_cast<uint32>(n)
        };
    }
}

ThreadPool::~ThreadPool()
{
    assert(Instance == this && "ThreadPool instance is not created!");
    Instance = nullptr;

    {
        std::scoped_lock lock(mutex);
        decltype(tasks) empty_queue;
        tasks.swap(empty_queue);
    }

    running = false;
    condition.notify_all();

    for (std::thread& thread : worker_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

void ThreadPool::WorkerLoop(uint32 thread_id)
{
    const std::u8string thread_name{
        utility::string_utils::ToU8String(std::format("Worker Thread {}", thread_id))
    };
    platform::Platform::SetCurrentThreadName(thread_name);

    while (true)
    {
        Function<void()> task;
        {
            std::unique_lock lock(mutex);

            // 작업이 없으면 대기
            condition.wait(lock, [this] { return !tasks.empty() || !running; });
            if (!running && tasks.empty())
            {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}
}
