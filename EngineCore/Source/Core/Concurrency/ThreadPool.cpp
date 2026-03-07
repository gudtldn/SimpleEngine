#include "SimpleEngine/Core/Concurrency/ThreadPool.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/StringUtils.h"


namespace se
{
ThreadPool::ThreadPool(String in_pool_name, uint32 num_threads)
    : pool_name(std::move(in_pool_name))
{
    ConsoleLog(ELogLevel::Info, "Initializing ThreadPool: [{}] (Count: {})", pool_name, num_threads);

    // Worker Thread 생성
    worker_threads.Reserve(num_threads);
    for (uint32 n = 0; n < num_threads; ++n)
    {
        worker_threads.Emplace([this, n](const std::stop_token& stoken)
        {
            WorkerLoop(stoken, n);
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::scoped_lock lock(mutex);
        if (!tasks.IsEmpty())
        {
            ConsoleLog(ELogLevel::Warning, "ThreadPool: [{}] destroyed. {} tasks discarded.", pool_name, tasks.Len());
            tasks.Clear();
        }
    }

    // 스레드 종료 요청
    for (std::jthread& thread : worker_threads)
    {
        thread.request_stop();
    }
    condition.notify_all();
    worker_threads.Clear();

    ConsoleLog(ELogLevel::Info, "ThreadPool: [{}] destroyed successfully.", pool_name);
}

void ThreadPool::WorkerLoop(const std::stop_token& stoken, uint32 thread_id)
{
    const String thread_name = String::Format("{} {}", pool_name, thread_id);
    Platform::SetCurrentThreadName(thread_name);

    while (true)
    {
        std::move_only_function<void()> current_task;
        {
            // stop이 요청되거나 작업이 생길 때까지 대기
            std::unique_lock lock(mutex);
            const bool has_task = condition.wait(lock, stoken, [this]
            {
                return !tasks.IsEmpty();
            });

            if (!has_task)
            {
                break;
            }

            current_task = std::move(tasks.Pop()).Value();
        }

        {
            ZoneScoped;
            ZoneName(thread_name.CStr(), thread_name.ByteLen());

            current_task();
        }
    }
}
}  // namespace se
