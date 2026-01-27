#include "Core/Concurrency/ThreadPool.h"

#include <cassert>

#include "Core/Logging/Logging.h"
#include "Utility/StringUtils.h"


namespace se::concurrency
{
ThreadPool::ThreadPool(String in_pool_name, uint32 num_threads)
    : pool_name(std::move(in_pool_name))
{
    ConsoleLog(ELogLevel::Info, "Initializing ThreadPool: [{}] (Count: {})", pool_name, num_threads);

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
    usize tasks_dropped = 0;
    {
        std::scoped_lock lock(mutex);
        tasks_dropped = tasks.Len();
        tasks.Clear();
    }

    // 스레드 중단 요청
    for (std::jthread& thread : worker_threads)
    {
        thread.request_stop();
    }
    condition.notify_all();
    worker_threads.Clear();

    if (tasks_dropped > 0)
    {
        ConsoleLog(ELogLevel::Warning, "ThreadPool: [{}] destroyed. {} tasks were discarded.", pool_name, tasks_dropped);
    }
    else
    {
        ConsoleLog(ELogLevel::Info, "ThreadPool: [{}] destroyed successfully.", pool_name);
    }
}

void ThreadPool::WorkerLoop(const std::stop_token& token, uint32 thread_id)
{
    const String thread_name = String::Format("{} {}", pool_name, thread_id);
    platform::SetCurrentThreadName(thread_name);

    while (!token.stop_requested())
    {
        Function<void()> task;
        {
            // stop이 요청되거나 작업이 생길 때까지 대기
            std::unique_lock lock(mutex);
            condition.wait(lock, [this, &token]
            {
                return !tasks.IsEmpty() || token.stop_requested();
            });

            if (tasks.IsEmpty() && token.stop_requested())
            {
                break;
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
}  // namespace se::concurrency
