#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include <memory>
#include <thread>

#include "SimpleEngine/Core/Concurrency/TaskScheduler.h"
#include "SimpleEngine/Core/Concurrency/ThreadPool.h"

using namespace se::core::concurrency;


// Global systems for the test environment, mimicking the Engine's ownership
static std::unique_ptr<ThreadPool> GThreadPool;
static std::unique_ptr<TaskScheduler> GTaskScheduler;

int main(int argc, char* argv[])
{
    // Initialize doctest
    doctest::Context context;
    context.applyCommandLine(argc, argv);

    // --- Initialize global systems before running tests ---
    // Using 2 threads for testing purposes
    GThreadPool = std::make_unique<ThreadPool>(2);
    GTaskScheduler = std::make_unique<TaskScheduler>(std::this_thread::get_id());
    // The constructors of these classes set the static `Instance` pointers, so `::Get()` will now work in all tests.

    // Run tests
    const int res = context.run();

    // --- Clean up global systems after tests are done ---
    // The order is important; TaskScheduler might use ThreadPool during its destruction.
    GTaskScheduler.reset();
    GThreadPool.reset();

    return res;
}
