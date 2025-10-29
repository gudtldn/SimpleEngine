#include "Core/Concurrency/ThreadPool.h"

#include <cassert>
#include <ranges>

#include "Core/Logging/Logging.h"
#include "Utility/StringUtils.h"


namespace se::core::concurrency
{
ThreadPool* ThreadPool::Instance = nullptr;

ThreadPool::ThreadPool(uint32 num_threads)
    : worker_threads(num_threads)
{
    assert(!Instance && "ThreadPool instance is already created!");
    Instance = this;

    ConsoleLog(ELogLevel::Info, "Creating ThreadPool...");

    // Worker Thread 생성
    for (auto [n, thread] : worker_threads | std::views::enumerate)
    {
        thread = std::jthread([this, id = static_cast<uint32>(n)](const std::stop_token& token)
        {
            WorkerLoop(token, id);
        });
    }
}

ThreadPool::~ThreadPool()
{
    assert(Instance == this && "ThreadPool instance is not created!");
    Instance = nullptr;

    ConsoleLog(ELogLevel::Info, "Destroying ThreadPool...");
    {
        std::scoped_lock lock(mutex);
        tasks.Clear();
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
    const String thread_name = String::Format("Worker Thread {}", thread_id);
    platform::SetCurrentThreadName(thread_name);

    while (!token.stop_requested())
    {
        Function<void()> task;
        {
            std::unique_lock lock(mutex);

            // stop이 요청되거나 작업이 생길 때까지 대기
            condition.wait(lock, [this, &token]
            {
                return !tasks.IsEmpty() || token.stop_requested();
            });

            if (token.stop_requested() && tasks.IsEmpty())
            {
                return;
            }

            task = std::move(*tasks.Pop());
        }

        {
            ZoneScoped;
            ZoneName(thread_name.CStr(), thread_name.ByteLen());

            task();
        }
    }
}
}
