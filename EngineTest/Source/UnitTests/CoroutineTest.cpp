#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include "SimpleEngine/Core/Concurrency/Coroutine.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/CoroutinePrimitives.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"

using namespace se;
using namespace std::chrono_literals;


// ═══════════════════════════════════════════════════════════════════
//  Test Fixture: JobSystem 기반 코루틴 테스트
// ═══════════════════════════════════════════════════════════════════

class CoroutineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        system = std::make_unique<JobSystem>(4);
    }

    void TearDown() override
    {
        system.reset();
    }

    /**
     * 코루틴이 final_suspend에 도달할 때까지 대기합니다.
     * 코루틴 프레임을 안전하게 소멸하기 위해 테스트 종료 전에 호출합니다.
     */
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

    std::unique_ptr<JobSystem> system;
};


// ═══════════════════════════════════════════════════════════════════
//  JobTaskPromise: JobAllocator 연동
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, Promise_AllocatedViaJobAllocator)
{
    // 코루틴 프레임이 JobAllocator를 통해 할당/해제됩니다.
    // 크래시 없이 생성/소멸되면 성공입니다.
    auto task = []() -> JobTask<int> { co_return 42; }();
    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_TRUE(task.handle.promise().storage.HasValue());
    EXPECT_EQ(task.handle.promise().storage.Value(), 42);
}


// ═══════════════════════════════════════════════════════════════════
//  JobTask: 기본 co_return
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, JobTask_CoReturnInt)
{
    auto task = []() -> JobTask<int> { co_return 123; }();
    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_EQ(task.handle.promise().storage.Value(), 123);
}

TEST_F(CoroutineTest, JobTask_CoReturnVoid)
{
    std::atomic<bool> executed = false;
    auto task = [&]() -> JobTask<void>
    {
        executed.store(true, std::memory_order_release);
        co_return;
    }();

    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
}


// ═══════════════════════════════════════════════════════════════════
//  JobTask: Symmetric Transfer (Zero-Cost co_await)
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, JobTask_SymmetricTransfer_NestedCoAwait)
{
    // 부모-자식 co_await가 Symmetric Transfer로 동작하여
    // JobSystem 큐잉 없이 동일 스레드에서 실행됩니다.
    auto task = []() -> JobTask<int>
    {
        auto child = []() -> JobTask<int> { co_return 42; };
        int val = co_await child();
        co_return val;
    }();

    task.handle.resume();

    // Symmetric Transfer: resume 한 번으로 parent+child 모두 완료
    EXPECT_TRUE(task.handle.done());
    EXPECT_EQ(task.handle.promise().storage.Value(), 42);
}

TEST_F(CoroutineTest, JobTask_SymmetricTransfer_DeepNesting)
{
    auto task = []() -> JobTask<int>
    {
        auto grandchild = []() -> JobTask<int> { co_return 10; };
        auto child = [&]() -> JobTask<int>
        {
            int val = co_await grandchild();
            co_return val * 2;
        };
        int val = co_await child();
        co_return val + 1;
    }();

    task.handle.resume();

    // parent -> child -> grandchild -> FinalAwaiter(child) -> FinalAwaiter(parent)
    // 전부 한 번의 resume 호출로 완료
    EXPECT_TRUE(task.handle.done());
    EXPECT_EQ(task.handle.promise().storage.Value(), 21);
}

TEST_F(CoroutineTest, JobTask_SymmetricTransfer_SameThread)
{
    // Symmetric Transfer는 스레드 전환 없이 동일 스레드에서 실행됩니다.
    std::thread::id parent_tid;
    std::thread::id child_tid;

    auto task = [&]() -> JobTask<void>
    {
        parent_tid = std::this_thread::get_id();

        auto child = [&]() -> JobTask<void>
        {
            child_tid = std::this_thread::get_id();
            co_return;
        };

        co_await child();
        // FinalAwaiter에 의해 부모로 돌아온 뒤에도 같은 스레드
        EXPECT_EQ(std::this_thread::get_id(), parent_tid);
        co_return;
    }();

    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_EQ(parent_tid, child_tid);
}

TEST_F(CoroutineTest, JobTask_SymmetricTransfer_VoidChild)
{
    std::atomic<bool> child_executed = false;

    auto task = [&]() -> JobTask<int>
    {
        auto void_child = [&]() -> JobTask<void>
        {
            child_executed.store(true, std::memory_order_release);
            co_return;
        };

        co_await void_child();
        co_return 99;
    }();

    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_TRUE(child_executed.load(std::memory_order_acquire));
    EXPECT_EQ(task.handle.promise().storage.Value(), 99);
}


// ═══════════════════════════════════════════════════════════════════
//  ResumeOn: 스레드 전환
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, ResumeOn_Worker)
{
    std::promise<std::thread::id> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await ResumeOn{ EJobThread::Worker };
        promise.set_value(std::this_thread::get_id());
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_NE(future.get(), std::this_thread::get_id());
    WaitForDone(task);
}

TEST_F(CoroutineTest, ResumeOn_Main)
{
    std::atomic<bool> executed = false;

    auto task = [&]() -> JobTask<void>
    {
        co_await ResumeOn{ EJobThread::Main };
        executed.store(true, std::memory_order_release);
        co_return;
    }();

    task.handle.resume();

    // 메인 큐를 드레인하기 전에는 실행되지 않습니다
    std::this_thread::sleep_for(10ms);
    EXPECT_FALSE(executed.load(std::memory_order_acquire));

    // 메인 스레드에서 큐를 드레인합니다
    JobSystem::Get().ExecuteMainThreadJobs();
    EXPECT_TRUE(executed.load(std::memory_order_acquire));

    while (!task.handle.done())
    {
        JobSystem::Get().ExecuteMainThreadJobs();
        std::this_thread::yield();
    }
}

TEST_F(CoroutineTest, ResumeOn_PreservesThreadAfterChildReturn)
{
    // ResumeOn으로 워커로 전환 -> 자식 co_await (symmetric transfer) ->
    // FinalAwaiter가 부모로 복귀 -> 부모는 여전히 같은 워커 스레드
    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await ResumeOn{ EJobThread::Worker };
        auto tid_before = std::this_thread::get_id();

        auto child = []() -> JobTask<int> { co_return 1; };
        co_await child();

        // Symmetric Transfer이므로 스레드가 유지됩니다
        promise.set_value(std::this_thread::get_id() == tid_before);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task);
}


// ═══════════════════════════════════════════════════════════════════
//  WhenAll
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, WhenAll_MultipleJobs)
{
    std::atomic<int> counter = 0;

    auto a = JobSystem::Get().Submit([&] { counter.fetch_add(1, std::memory_order_relaxed); });
    auto b = JobSystem::Get().Submit([&] { counter.fetch_add(10, std::memory_order_relaxed); });
    auto c = JobSystem::Get().Submit([&] { counter.fetch_add(100, std::memory_order_relaxed); });

    std::promise<int> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAll{ { a, b, c } };
        promise.set_value(counter.load(std::memory_order_relaxed));
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(future.get(), 111);
    WaitForDone(task);
}

TEST_F(CoroutineTest, WhenAll_AlreadyComplete)
{
    auto handle = JobSystem::Get().Submit([] {});
    handle.Wait();

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAll{ { handle } };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task);
}

TEST_F(CoroutineTest, WhenAll_Empty)
{
    auto task = []() -> JobTask<int>
    {
        co_await WhenAll{ {} };
        co_return 1;
    }();

    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_EQ(task.handle.promise().storage.Value(), 1);
}


TEST_F(CoroutineTest, WhenAll_AllAlreadyComplete)
{
    // 모든 핸들이 이미 완료된 상태에서 WhenAll 호출 (사전 카운트 레이스 컨디션 검증)
    auto h1 = JobSystem::Get().Submit([] {});
    auto h2 = JobSystem::Get().Submit([] {});
    auto h3 = JobSystem::Get().Submit([] {});

    h1.Wait();
    h2.Wait();
    h3.Wait();

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAll{ { h1, h2, h3 } };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task);
}

TEST_F(CoroutineTest, WhenAll_MixedComplete)
{
    // 일부는 이미 완료, 일부는 미완료인 핸들로 WhenAll 호출
    auto already_done = JobSystem::Get().Submit([] {});
    already_done.Wait();

    std::atomic<int> counter = 0;
    auto pending = JobSystem::Get().Submit([&] { counter.fetch_add(1, std::memory_order_relaxed); });

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAll{ { already_done, pending } };
        promise.set_value(counter.load(std::memory_order_relaxed) == 1);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task);
}


// ═══════════════════════════════════════════════════════════════════
//  WhenAny
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, WhenAny_FirstComplete)
{
    std::atomic<bool> slow_done = false;

    auto fast = JobSystem::Get().Submit([] {});
    auto slow = JobSystem::Get().Submit([&]
    {
        while (!slow_done.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
    });

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAny{ { fast, slow } };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());

    slow_done.store(true, std::memory_order_release);
    slow.Wait();
    WaitForDone(task);
}

TEST_F(CoroutineTest, WhenAny_ConcurrentStress)
{
    // 다수의 핸들을 동시에 Submit하고 WhenAny 호출 → 정확히 1회만 resume되는지 검증
    constexpr usize HANDLE_COUNT = 100;

    Array<JobHandle> handles;
    handles.Reserve(HANDLE_COUNT);
    for (usize i = 0; i < HANDLE_COUNT; ++i)
    {
        handles.Push(JobSystem::Get().Submit([] { std::this_thread::yield(); }));
    }

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAny{ std::move(handles) };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task);
}

TEST_F(CoroutineTest, WhenAny_AlreadyComplete)
{
    auto handle = JobSystem::Get().Submit([] {});
    handle.Wait();

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAny{ { handle } };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task);
}

TEST_F(CoroutineTest, WhenAny_EmptyArray_MustCompleteImmediately)
{
    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAny{ {} };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    // 빈 WhenAny는 즉시 통과해야 한다.
    ASSERT_EQ(future.wait_for(200ms), std::future_status::ready)
        << "WhenAny with empty handles must not suspend forever";
    EXPECT_TRUE(future.get());
    EXPECT_TRUE(task.handle.done());
}

TEST_F(CoroutineTest, WhenAny_AllInvalidHandles_MustNotSuspend)
{
    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        JobHandle invalid_a;
        JobHandle invalid_b;
        co_await WhenAny{ { invalid_a, invalid_b } };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(200ms), std::future_status::ready)
        << "WhenAny with only invalid handles should not enter permanent suspend";
    EXPECT_TRUE(future.get());
    EXPECT_TRUE(task.handle.done());
}

TEST_F(CoroutineTest, WhenAny_MixedInvalidAndPending_ResumesOnPendingCompletion)
{
    std::atomic<bool> run = false;
    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle invalid;
    auto pending = JobSystem::Get().Submit([&run]
    {
        while (!run.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
    });

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAny{ { invalid, pending } };
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    EXPECT_EQ(future.wait_for(50ms), std::future_status::timeout)
        << "WhenAny must not complete before pending handle is done";

    run.store(true, std::memory_order_release);
    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    pending.Wait();
    WaitForDone(task);
}

TEST_F(CoroutineTest, WhenAny_ResumeExactlyOnce_UnderBurstCompletion)
{
    constexpr usize HANDLE_COUNT = 256;

    Array<JobHandle> handles;
    handles.Reserve(HANDLE_COUNT);

    std::atomic<bool> release = false;
    for (usize i = 0; i < HANDLE_COUNT; ++i)
    {
        handles.Push(JobSystem::Get().Submit([&release]
        {
            while (!release.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
        }));
    }

    std::atomic<int> resume_count = 0;
    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await WhenAny{ std::move(handles) };
        const int count = resume_count.fetch_add(1, std::memory_order_relaxed) + 1;
        promise.set_value(count == 1);
        co_return;
    }();

    task.handle.resume();
    release.store(true, std::memory_order_release);

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    EXPECT_EQ(resume_count.load(std::memory_order_relaxed), 1);
    WaitForDone(task);
}


// ═══════════════════════════════════════════════════════════════════
//  ParallelFor (co_await JobHandle 직접 사용)
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, ParallelFor_CoAwaitJobHandle)
{
    constexpr usize COUNT = 1000;
    std::atomic<usize> sum = 0;

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await JobSystem::Get().ParallelFor(COUNT, 100, [&](usize i)
        {
            sum.fetch_add(i, std::memory_order_relaxed);
        });
        promise.set_value(true);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(sum.load(std::memory_order_relaxed), static_cast<usize>(499500));
    WaitForDone(task);
}

TEST_F(CoroutineTest, CoAwait_TemporaryJobHandle)
{
    std::atomic<int> value = 0;
    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        co_await JobSystem::Get().Submit([&value]
        {
            value.store(42, std::memory_order_release);
        });

        promise.set_value(value.load(std::memory_order_acquire) == 42);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task);
}

TEST_F(CoroutineTest, DeepChain_CoAwaitJobHandle)
{
    constexpr usize CHAIN_DEPTH = 2048;
    std::atomic<usize> progressed = 0;

    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [&]() -> JobTask<void>
    {
        for (usize i = 0; i < CHAIN_DEPTH; ++i)
        {
            co_await JobSystem::Get().Submit([&progressed]
            {
                progressed.fetch_add(1, std::memory_order_relaxed);
            });
        }

        promise.set_value(progressed.load(std::memory_order_relaxed) == CHAIN_DEPTH);
        co_return;
    }();

    task.handle.resume();

    ASSERT_EQ(future.wait_for(10s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    WaitForDone(task, 10s);
}
