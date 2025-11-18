#include "Core/Concurrency/ThreadPool.h"

#include <cassert>

#include "Core/Logging/Logging.h"
#include "Utility/StringUtils.h"


namespace se::concurrency
{
ThreadPool::ThreadPool(String in_pool_name, uint32 num_threads)
    : pool_name(std::move(in_pool_name))
{
    ConsoleLog(ELogLevel::Info, "Creating ThreadPool...");

    // Worker Thread 생성
    worker_threads.Reserve(num_threads);
    for (uint32 n = 0; n < num_threads; ++n)
    {
        worker_threads.Emplace([this, n](const std::stop_token& token)
        {
            WorkerLoop(token, n);
        });
    }
}

ThreadPool::~ThreadPool()
{
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
    const String thread_name = String::Format("{} {}", pool_name, thread_id);
    platform::SetCurrentThreadName(thread_name);

    while (!token.stop_requested())
    {
        core::Function<void()> task;
        {
            // stop이 요청되거나 작업이 생길 때까지 대기
            std::unique_lock lock(mutex);
            condition.wait(lock, [this, &token]
            {
                return !tasks.IsEmpty() || token.stop_requested();
            });

            if (tasks.IsEmpty() && token.stop_requested())
            {
                return;
            }

            task = std::move(tasks.Pop()).Value();
        }

        {
            ZoneScoped;
            ZoneName(thread_name.CStr(), thread_name.ByteLen());

            task();
        }
    }
}
}
