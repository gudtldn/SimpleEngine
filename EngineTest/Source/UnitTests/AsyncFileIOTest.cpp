#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>

#include "SimpleEngine/Core/Concurrency/AsyncFileIO.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/CoroutinePrimitives.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/JobTask.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Types/Path.h"

using namespace se;
using namespace std::chrono_literals;


// ═══════════════════════════════════════════════════════════════════
//  Test Fixture: AsyncFileIO 기반 비동기 I/O 테스트
// ═══════════════════════════════════════════════════════════════════

class AsyncFileIOTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        job_system = std::make_unique<JobSystem>(4);
        async_io = std::make_unique<AsyncFileIO>();

        // 테스트용 임시 파일 생성
        test_file_path = (std::filesystem::temp_directory_path() / "se_asyncio_test.txt").string();
        std::ofstream ofs(test_file_path);
        ofs << TEST_CONTENT;
        ofs.close();
    }

    void TearDown() override
    {
        async_io.reset();
        job_system.reset();

        // 임시 파일 정리
        std::filesystem::remove(test_file_path);
    }

    std::unique_ptr<JobSystem> job_system;
    std::unique_ptr<AsyncFileIO> async_io;

    std::string test_file_path;
    static constexpr const char* TEST_CONTENT = "Hello, AsyncIO!";
};


// ═══════════════════════════════════════════════════════════════════
//  싱글톤 기본 동작
// ═══════════════════════════════════════════════════════════════════

TEST_F(AsyncFileIOTest, SingletonLifetime)
{
    EXPECT_TRUE(AsyncFileIO::IsInitialized());
    EXPECT_NO_THROW(AsyncFileIO::Get());
}


// ═══════════════════════════════════════════════════════════════════
//  ReadFile: 콜백 기반 비동기 읽기
// ═══════════════════════════════════════════════════════════════════

TEST_F(AsyncFileIOTest, ReadFile_Callback_Success)
{
    std::promise<IOResult> promise;
    auto future = promise.get_future();

    async_io->ReadFile(Path{ test_file_path.c_str() }, [&promise](IOResult result)
    {
        promise.set_value(std::move(result));
    });

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    const IOResult result = future.get();

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.data.Len(), std::strlen(TEST_CONTENT));
    EXPECT_EQ(std::memcmp(result.data.Data(), TEST_CONTENT, result.data.Len()), 0);
}

TEST_F(AsyncFileIOTest, ReadFile_Callback_NonExistent)
{
    std::promise<IOResult> promise;
    auto future = promise.get_future();

    async_io->ReadFile("__non_existent_file_12345__.txt", [&promise](IOResult result)
    {
        promise.set_value(std::move(result));
    });

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    const IOResult result = future.get();

    EXPECT_FALSE(result.success);
}

TEST_F(AsyncFileIOTest, ReadFile_Callback_ExecutedOnWorker)
{
    // 콜백이 Poller Thread가 아닌 Worker 스레드에서 실행되는지 확인
    std::promise<std::thread::id> promise;
    auto future = promise.get_future();

    const auto main_thread_id = std::this_thread::get_id();

    async_io->ReadFile(Path{ test_file_path.c_str() }, [&promise](IOResult)
    {
        promise.set_value(std::this_thread::get_id());
    });

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    const auto callback_thread_id = future.get();

    // 콜백은 메인 스레드에서 실행되지 않아야 한다 (Worker에서 실행)
    EXPECT_NE(callback_thread_id, main_thread_id);
}


// ═══════════════════════════════════════════════════════════════════
//  ReadFileAsync: 코루틴 기반 비동기 읽기
// ═══════════════════════════════════════════════════════════════════

TEST_F(AsyncFileIOTest, ReadFileAsync_Success)
{
    IOResult captured_result;

    JobHandle h = JobSystem::Get().SubmitTask([](std::string path, IOResult& captured_result) static -> JobTask<void>
    {
        IOResult result = co_await AsyncFileIO::Get().ReadFileAsync(Path{ path.c_str() });
        captured_result = std::move(result);
        co_return;
    }(test_file_path, captured_result));

    h.Wait();

    EXPECT_TRUE(captured_result.success);
    EXPECT_EQ(captured_result.data.Len(), std::strlen(TEST_CONTENT));
    EXPECT_EQ(std::memcmp(captured_result.data.Data(), TEST_CONTENT, captured_result.data.Len()), 0);
}

TEST_F(AsyncFileIOTest, ReadFileAsync_NonExistent)
{
    std::atomic<bool> done = false;
    IOResult captured_result;

    JobHandle h = JobSystem::Get().SubmitTask(
        [](IOResult& captured_result, std::atomic<bool>& done) static -> JobTask<void>
        {
            IOResult result = co_await AsyncFileIO::Get().ReadFileAsync("__non_existent_file_99999__.txt");
            captured_result = std::move(result);
            done.store(true, std::memory_order_release);
        }(captured_result, done));

    const auto start = std::chrono::steady_clock::now();
    while (!done.load(std::memory_order_acquire))
    {
        if (std::chrono::steady_clock::now() - start > 5s)
        {
            FAIL() << "ReadFileAsync(non-existent) timed out";
        }
        std::this_thread::yield();
    }

    h.Wait();

    EXPECT_FALSE(captured_result.success);
}


// ═══════════════════════════════════════════════════════════════════
//  동시 다중 요청
// ═══════════════════════════════════════════════════════════════════

TEST_F(AsyncFileIOTest, ReadFile_MultipleRequests)
{
    constexpr int NUM_REQUESTS = 10;
    std::atomic<int> completed_count = 0;

    for (int i = 0; i < NUM_REQUESTS; ++i)
    {
        async_io->ReadFile(Path{ test_file_path.c_str() }, [&completed_count](IOResult result)
        {
            if (result.success)
            {
                completed_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    const auto start = std::chrono::steady_clock::now();
    while (completed_count.load(std::memory_order_relaxed) < NUM_REQUESTS)
    {
        if (std::chrono::steady_clock::now() - start > 10s)
        {
            FAIL() << "Multiple requests timed out. Completed: " << completed_count.load();
        }
        std::this_thread::yield();
    }

    EXPECT_EQ(completed_count.load(), NUM_REQUESTS);
}

TEST_F(AsyncFileIOTest, ReadFileAsync_MultipleCoroutines)
{
    constexpr int NUM_REQUESTS = 5;
    std::atomic<int> completed_count = 0;

    Array<JobHandle> handles;
    handles.Reserve(NUM_REQUESTS);

    for (int i = 0; i < NUM_REQUESTS; ++i)
    {
        handles.Push(JobSystem::Get().SubmitTask(
            [](std::string path, std::atomic<int>& count) static -> JobTask<void>
            {
                IOResult result = co_await AsyncFileIO::Get().ReadFileAsync(Path{ path.c_str() });
                if (result.success)
                {
                    count.fetch_add(1, std::memory_order_relaxed);
                }
            }(test_file_path, completed_count)));
    }

    const auto start = std::chrono::steady_clock::now();
    while (completed_count.load(std::memory_order_relaxed) < NUM_REQUESTS)
    {
        if (std::chrono::steady_clock::now() - start > 10s)
        {
            FAIL() << "Multiple coroutines timed out. Completed: " << completed_count.load();
        }
        std::this_thread::yield();
    }

    // 모든 코루틴이 정상 종료될 때까지 대기
    for (const auto& h : handles)
    {
        h.Wait();
    }

    EXPECT_EQ(completed_count.load(), NUM_REQUESTS);
}
