#include "gtest/gtest.h"

#include <chrono>
#include <future>
#include <thread>

#include "SimpleEngine/Core/Concurrency/Coroutine.h"
#include "SimpleEngine/Core/Concurrency/TaskScheduler.h"
#include "SimpleEngine/Core/Concurrency/ThreadPool.h"



class ConcurrencyTest : public ::testing::Test{};

TEST_F(ConcurrencyTest, ThreadPoolExecutesSimpleTask)
{
    auto thread_pool = ThreadPool("Worker Thread", 2);

    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();

    // Submit a task that sets the promise value to true
    thread_pool.Submit([&promise]
    {
        promise.set_value(true);
    });

    // Wait for the future to be ready
    using namespace std::chrono_literals;
    const auto status = future.wait_for(1s);

    ASSERT_EQ(status, std::future_status::ready) << "Task did not complete within the 1s timeout.";
    EXPECT_TRUE(future.get());
}

TEST_F(ConcurrencyTest, TaskSchedulerLaunchesOnWorkerThread)
{
    std::promise<std::thread::id> promise;
    std::future<std::thread::id> future = promise.get_future();

    // 작업 스케줄러를 통해 워커 스레드에서 코루틴 작업 실행
    TaskScheduler::Get().Launch_WorkerThread([&promise]() -> Task<void>
    {
        promise.set_value(std::this_thread::get_id());
        co_return;
    }());

    using namespace std::chrono_literals;
    const auto status = future.wait_for(1s);

    ASSERT_EQ(status, std::future_status::ready)
        << "Coroutine task did not complete within the 1s timeout.";

    const auto task_thread_id = future.get();
    const auto main_thread_id = std::this_thread::get_id();

    EXPECT_NE(task_thread_id, main_thread_id)
        << "Task was executed on the main thread instead of a worker thread.";
}
