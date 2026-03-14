#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>

#include "SimpleEngine/Core/Concurrency/AsyncFileIO.h"
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
        ofs << test_content;
        ofs.close();
    }

    void TearDown() override
    {
        async_io.reset();
        job_system.reset();

        // 임시 파일 정리
        std::filesystem::remove(test_file_path);
    }

    template <typename T>
    static void WaitForDone(const JobTask<T>& task, std::chrono::milliseconds timeout = 5s)
    {
        const auto start = std::chrono::steady_clock::now();
        while (!task.handle.done())
        {
            if (std::chrono::steady_clock::now() - start > timeout)
            {
                FAIL() << "코루틴이 타임아웃 내에 완료되지 않았습니다.";
            }
            std::this_thread::yield();
        }
    }

    std::unique_ptr<JobSystem> job_system;
    std::unique_ptr<AsyncFileIO> async_io;

    std::string test_file_path;
    static constexpr const char* test_content = "Hello, AsyncIO!";
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
    EXPECT_EQ(result.bytes_transferred, std::strlen(test_content));
    EXPECT_EQ(std::memcmp(result.data.get(), test_content, result.bytes_transferred), 0);
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
    std::atomic<bool> done = false;
    IOResult captured_result;

    auto task = [&]() -> JobTask<void>
    {
        IOResult result = co_await AsyncFileIO::Get().ReadFileAsync(Path{ test_file_path.c_str() });
        captured_result = std::move(result);
        done.store(true, std::memory_order_release);
    }();

    // 코루틴을 시작 (Lazy Start이므로 수동 resume 필요)
    task.handle.resume();

    const auto start = std::chrono::steady_clock::now();
    while (!done.load(std::memory_order_acquire))
    {
        if (std::chrono::steady_clock::now() - start > 5s)
        {
            FAIL() << "ReadFileAsync 타임아웃";
        }
        std::this_thread::yield();
    }

    WaitForDone(task);

    EXPECT_TRUE(captured_result.success);
    EXPECT_EQ(captured_result.bytes_transferred, std::strlen(test_content));
    EXPECT_EQ(std::memcmp(captured_result.data.get(), test_content, captured_result.bytes_transferred), 0);
}

TEST_F(AsyncFileIOTest, ReadFileAsync_NonExistent)
{
    std::atomic<bool> done = false;
    IOResult captured_result;

    auto task = [&]() -> JobTask<void>
    {
        IOResult result = co_await AsyncFileIO::Get().ReadFileAsync("__non_existent_file_99999__.txt");
        captured_result = std::move(result);
        done.store(true, std::memory_order_release);
    }();

    task.handle.resume();

    const auto start = std::chrono::steady_clock::now();
    while (!done.load(std::memory_order_acquire))
    {
        if (std::chrono::steady_clock::now() - start > 5s)
        {
            FAIL() << "ReadFileAsync(non-existent) 타임아웃";
        }
        std::this_thread::yield();
    }

    WaitForDone(task);

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
            FAIL() << "다중 요청 타임아웃. 완료: " << completed_count.load();
        }
        std::this_thread::yield();
    }

    EXPECT_EQ(completed_count.load(), NUM_REQUESTS);
}

TEST_F(AsyncFileIOTest, ReadFileAsync_MultipleCoroutines)
{
    constexpr int NUM_REQUESTS = 5;
    std::atomic<int> completed_count = 0;

    Array<JobTask<void>> tasks;
    tasks.Reserve(NUM_REQUESTS);

    for (int i = 0; i < NUM_REQUESTS; ++i)
    {
        tasks.Emplace([&]() -> JobTask<void>
        {
            IOResult result = co_await AsyncFileIO::Get().ReadFileAsync(Path{ test_file_path.c_str() });
            if (result.success)
            {
                completed_count.fetch_add(1, std::memory_order_relaxed);
            }
        }());
    }

    // 모든 코루틴을 시작
    for (auto& task : tasks)
    {
        task.handle.resume();
    }

    const auto start = std::chrono::steady_clock::now();
    while (completed_count.load(std::memory_order_relaxed) < NUM_REQUESTS)
    {
        if (std::chrono::steady_clock::now() - start > 10s)
        {
            FAIL() << "다중 코루틴 타임아웃. 완료: " << completed_count.load();
        }
        std::this_thread::yield();
    }

    // 모든 코루틴이 정상 종료될 때까지 대기
    for (const auto& task : tasks)
    {
        WaitForDone(task);
    }

    EXPECT_EQ(completed_count.load(), NUM_REQUESTS);
}
