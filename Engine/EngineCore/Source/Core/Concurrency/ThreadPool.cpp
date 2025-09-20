module SE.Core;
import :Concurrency.ThreadPool;

using namespace se::core::function;


namespace se::core::concurrency
{
ThreadPool::ThreadPool(uint32 num_threads)
    : worker_threads(num_threads)
{
    // Worker Thread 생성
    for (std::thread& thread : worker_threads)
    {
        thread = std::thread(&ThreadPool::WorkerLoop, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::scoped_lock lock(mutex);
        running = false;
    }

    condition.notify_all();

    for (std::thread& thread : worker_threads)
    {
        thread.join();
    }
}

void ThreadPool::Submit(Function<void()> task)
{
    {
        std::scoped_lock lock(mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::WorkerLoop()
{
    while (true)
    {
        Function<void()> task;
        {
            std::unique_lock lock(mutex);

            // 작업이 없으면 대기
            condition.wait(lock, [this] { return !tasks.empty() || !running; });
            if (!running)
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
