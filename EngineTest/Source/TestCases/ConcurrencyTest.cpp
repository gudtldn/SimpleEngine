#include "doctest/doctest.h"

#include <chrono>
#include <future>
#include <thread>

#include "SimpleEngine/Core/Concurrency/Coroutine.h"
#include "SimpleEngine/Core/Concurrency/TaskScheduler.h"
#include "SimpleEngine/Core/Concurrency/ThreadPool.h"

using namespace se::core::concurrency;


TEST_SUITE (
"SimpleEngine.Core.Concurrency"
)
{
TEST_CASE("ThreadPool can execute a simple task")
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();

    // Submit a task that sets the promise value to true
    ThreadPool::SubmitTask([&promise]
    {
        promise.set_value(true);
    });

    // Wait for the future to be ready
    using namespace std::chrono_literals;
    const auto status = future.wait_for(1s); // 1-second timeout to prevent infinite hang

    REQUIRE(status == std::future_status::ready);
    CHECK(future.get() == true);
}

TEST_CASE("TaskScheduler can launch a task on a worker thread")
{
    std::promise<std::thread::id> promise;
    std::future<std::thread::id> future = promise.get_future();

    // Launch a task that captures the thread ID
    TaskScheduler::Get().Launch_WorkerThread([&promise]() -> Task<void>
    {
        promise.set_value(std::this_thread::get_id());
        co_return;
    }());

    using namespace std::chrono_literals;
    const auto status = future.wait_for(1s);

    REQUIRE(status == std::future_status::ready);
    // Check that the task ran on a different thread than the main test thread
    CHECK(future.get() != std::this_thread::get_id());
}
}
