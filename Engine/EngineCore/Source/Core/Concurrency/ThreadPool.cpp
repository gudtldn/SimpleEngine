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

    ConsoleLog(ELogLevel::Info, u8"Creating ThreadPool...");

    // Worker Thread 생성
    for (auto [n, thread] : worker_threads | std::views::enumerate)
    {
        thread = std::jthread{
            [this, n = static_cast<uint32>(n)](const std::stop_token& token)
            {
                WorkerLoop(token, n);
            },
        };
    }
}

ThreadPool::~ThreadPool()
{
    assert(Instance == this && "ThreadPool instance is not created!");
    Instance = nullptr;

    ConsoleLog(ELogLevel::Info, u8"Destroying ThreadPool...");
    {
        std::scoped_lock lock(mutex);
        decltype(tasks) empty_queue;
        tasks.swap(empty_queue);
    }

    // 스레드 중단
    for (std::jthread& thread : worker_threads)
    {
        thread.request_stop();
    }
    condition.notify_all();
}

void ThreadPool::WorkerLoop(const std::stop_token& token, uint32 thread_id)
{
    const u8string thread_name = utility::string_utils::ToU8String(
        std::format("Worker Thread {}", thread_id)
    );
    platform::Platform::SetCurrentThreadName(thread_name);

    while (!token.stop_requested())
    {
        Function<void()> task;
        {
            std::unique_lock lock(mutex);

            // stop이 요청되거나 작업이 생길 때까지 대기
            condition.wait(lock, [this, &token]
            {
                return !tasks.empty() || token.stop_requested();
            });

            if (token.stop_requested() && tasks.empty())
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
