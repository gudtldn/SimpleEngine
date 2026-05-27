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

    std::unique_ptr<JobSystem> system;
};


// ═══════════════════════════════════════════════════════════════════
//  JobTaskPromise: JobAllocator 연동
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, Promise_AllocatedViaJobAllocator)
{
    // 코루틴 프레임이 JobAllocator를 통해 할당/해제됩니다.
    // 크래시 없이 생성/소멸되면 성공입니다.
    auto task = []() static -> JobTask<int> { co_return 42; }();
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
    auto task = []() static -> JobTask<int> { co_return 123; }();
    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_EQ(task.handle.promise().storage.Value(), 123);
}

TEST_F(CoroutineTest, JobTask_CoReturnVoid)
{
    std::atomic<bool> executed = false;
    auto task = [](std::atomic<bool>& executed) static -> JobTask<void>
    {
        executed.store(true, std::memory_order_release);
        co_return;
    }(executed);

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
    auto task = []() static -> JobTask<int>
    {
        auto child = []() static -> JobTask<int> { co_return 42; };
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
    auto task = []() static -> JobTask<int>
    {
        auto child = []() static -> JobTask<int>
        {
            auto grandchild = []() static -> JobTask<int> { co_return 10; };
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

    auto task = [](std::thread::id& parent_tid, std::thread::id& child_tid) static -> JobTask<void>
    {
        parent_tid = std::this_thread::get_id();

        auto child = [](std::thread::id& tid) static -> JobTask<void>
        {
            tid = std::this_thread::get_id();
            co_return;
        };

        co_await child(child_tid);
        // FinalAwaiter에 의해 부모로 돌아온 뒤에도 같은 스레드
        EXPECT_EQ(std::this_thread::get_id(), parent_tid);
        co_return;
    }(parent_tid, child_tid);

    task.handle.resume();

    EXPECT_TRUE(task.handle.done());
    EXPECT_EQ(parent_tid, child_tid);
}

TEST_F(CoroutineTest, JobTask_SymmetricTransfer_VoidChild)
{
    std::atomic<bool> child_executed = false;

    auto task = [](std::atomic<bool>& child_executed) static -> JobTask<int>
    {
        auto void_child = [](std::atomic<bool>& flag) static -> JobTask<void>
        {
            flag.store(true, std::memory_order_release);
            co_return;
        };

        co_await void_child(child_executed);
        co_return 99;
    }(child_executed);

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

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<std::thread::id>& p) static -> JobTask<void>
        {
            co_await ResumeOn{ EJobThread::Worker };
            p.set_value(std::this_thread::get_id());
            co_return;
        }(promise));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_NE(future.get(), std::this_thread::get_id());
    h.Wait();
}

TEST_F(CoroutineTest, ResumeOn_Main)
{
    std::atomic<bool> executed = false;

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::atomic<bool>& executed) static -> JobTask<void>
        {
            co_await ResumeOn{ EJobThread::Main };
            executed.store(true, std::memory_order_release);
            co_return;
        }(executed));

    // 메인 큐를 드레인하기 전에는 실행되지 않습니다
    std::this_thread::sleep_for(10ms);
    EXPECT_FALSE(executed.load(std::memory_order_acquire));

    // 메인 스레드에서 큐를 드레인합니다
    JobSystem::Get().ExecuteMainThreadJobs();
    EXPECT_TRUE(executed.load(std::memory_order_acquire));

    while (!h.IsComplete())
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

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p) static -> JobTask<void>
        {
            co_await ResumeOn{ EJobThread::Worker };
            auto tid_before = std::this_thread::get_id();

            auto child = []() static -> JobTask<int> { co_return 1; };
            co_await child();

            // Symmetric Transfer이므로 스레드가 유지됩니다
            p.set_value(std::this_thread::get_id() == tid_before);
            co_return;
        }(promise));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
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

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<int>& p, std::atomic<int>& cnt, JobHandle<void> a, JobHandle<void> b, JobHandle<void> c) static -> JobTask<void>
        {
            co_await WhenAll{ { a, b, c } };
            p.set_value(cnt.load(std::memory_order_relaxed));
            co_return;
        }(promise, counter, a, b, c));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(future.get(), 111);
    h.Wait();
}

TEST_F(CoroutineTest, WhenAll_AlreadyComplete)
{
    auto handle = JobSystem::Get().Submit([] {});
    handle.Wait();

    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, JobHandle<void> handle) static -> JobTask<void>
        {
            co_await WhenAll{ { handle } };
            p.set_value(true);
            co_return;
        }(promise, handle));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
}

TEST_F(CoroutineTest, WhenAll_Empty)
{
    auto task = []() static -> JobTask<int>
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

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, JobHandle<void> h1, JobHandle<void> h2, JobHandle<void> h3) static -> JobTask<void>
        {
            co_await WhenAll{ { h1, h2, h3 } };
            p.set_value(true);
            co_return;
        }(promise, h1, h2, h3));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
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

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, std::atomic<int>& cnt, JobHandle<void> done, JobHandle<void> pend) static -> JobTask<void>
        {
            co_await WhenAll{ { done, pend } };
            p.set_value(cnt.load(std::memory_order_relaxed) == 1);
            co_return;
        }(promise, counter, already_done, pending));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
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

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, JobHandle<void> fast, JobHandle<void> slow) static -> JobTask<void>
        {
            co_await WhenAny{ { fast, slow } };
            p.set_value(true);
            co_return;
        }(promise, fast, slow));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());

    slow_done.store(true, std::memory_order_release);
    slow.Wait();
    h.Wait();
}

TEST_F(CoroutineTest, WhenAny_ConcurrentStress)
{
    // 다수의 핸들을 동시에 Submit하고 WhenAny 호출 -> 정확히 1회만 resume되는지 검증
    constexpr usize HANDLE_COUNT = 100;

    Array<JobHandle<void>> handles;
    handles.Reserve(HANDLE_COUNT);
    for (usize i = 0; i < HANDLE_COUNT; ++i)
    {
        handles.Push(JobSystem::Get().Submit([] { std::this_thread::yield(); }));
    }

    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, Array<JobHandle<void>> handles) static -> JobTask<void>
        {
            co_await WhenAny{ std::move(handles) };
            p.set_value(true);
            co_return;
        }(promise, std::move(handles)));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
}

TEST_F(CoroutineTest, WhenAny_AlreadyComplete)
{
    auto handle = JobSystem::Get().Submit([] {});
    handle.Wait();

    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, JobHandle<void> handle) static -> JobTask<void>
        {
            co_await WhenAny{ { handle } };
            p.set_value(true);
            co_return;
        }(promise, handle));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
}

TEST_F(CoroutineTest, WhenAny_EmptyArray_MustCompleteImmediately)
{
    std::promise<bool> promise;
    auto future = promise.get_future();

    auto task = [](std::promise<bool>& p) static -> JobTask<void>
    {
        co_await WhenAny{ {} };
        p.set_value(true);
        co_return;
    }(promise);

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

    auto task = [](std::promise<bool>& p) static -> JobTask<void>
    {
        JobHandle<void> invalid_a;
        JobHandle<void> invalid_b;
        co_await WhenAny{ { invalid_a, invalid_b } };
        p.set_value(true);
        co_return;
    }(promise);

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

    JobHandle<void> invalid;
    auto pending = JobSystem::Get().Submit([&run]
    {
        while (!run.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
    });

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, JobHandle<void> invalid, JobHandle<void> pending) static -> JobTask<void>
        {
            co_await WhenAny{ { invalid, pending } };
            p.set_value(true);
            co_return;
        }(promise, invalid, pending));

    EXPECT_EQ(future.wait_for(50ms), std::future_status::timeout)
        << "WhenAny must not complete before pending handle is done";

    run.store(true, std::memory_order_release);
    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    pending.Wait();
    h.Wait();
}

TEST_F(CoroutineTest, WhenAny_ResumeExactlyOnce_UnderBurstCompletion)
{
    constexpr usize HANDLE_COUNT = 256;

    Array<JobHandle<void>> handles;
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

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, std::atomic<int>& rc, Array<JobHandle<void>> handles) static -> JobTask<void>
        {
            co_await WhenAny{ std::move(handles) };
            const int count = rc.fetch_add(1, std::memory_order_relaxed) + 1;
            p.set_value(count == 1);
            co_return;
        }(promise, resume_count, std::move(handles)));

    release.store(true, std::memory_order_release);

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    EXPECT_EQ(resume_count.load(std::memory_order_relaxed), 1);
    h.Wait();
}


// ═══════════════════════════════════════════════════════════════════
//  ParallelFor (co_await JobHandle 직접 사용)
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, ParallelFor_CoAwaitJobHandle)
{
    std::atomic<usize> sum = 0;

    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::atomic<usize>& sum, std::promise<bool>& p) static -> JobTask<void>
        {
            constexpr usize COUNT = 1000;
            co_await JobSystem::Get().ParallelFor(COUNT, static_cast<usize>(100), [&sum](usize i)
            {
                sum.fetch_add(i, std::memory_order_relaxed);
            });
            p.set_value(true);
            co_return;
        }(sum, promise));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(sum.load(std::memory_order_relaxed), static_cast<usize>(499500));
    h.Wait();
}

TEST_F(CoroutineTest, CoAwait_TemporaryJobHandle)
{
    std::atomic<int> value = 0;
    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::atomic<int>& value, std::promise<bool>& p) static -> JobTask<void>
        {
            co_await JobSystem::Get().Submit([&value]
            {
                value.store(42, std::memory_order_release);
            });

            p.set_value(value.load(std::memory_order_acquire) == 42);
            co_return;
        }(value, promise));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
}

TEST_F(CoroutineTest, DeepChain_CoAwaitJobHandle)
{
    std::atomic<usize> progressed = 0;

    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::atomic<usize>& prog, std::promise<bool>& p) static -> JobTask<void>
        {
            constexpr usize CHAIN_DEPTH = 2048;
            for (usize i = 0; i < CHAIN_DEPTH; ++i)
            {
                co_await JobSystem::Get().Submit([&prog]
                {
                    prog.fetch_add(1, std::memory_order_relaxed);
                });
            }

            p.set_value(prog.load(std::memory_order_relaxed) == CHAIN_DEPTH);
            co_return;
        }(progressed, promise));

    ASSERT_EQ(future.wait_for(10s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    h.Wait();
}


// ═══════════════════════════════════════════════════════════════════
//  SubmitTask / DispatchTask: Launch API
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, SubmitTask_TrackedCompletion)
{
    // SubmitTask로 제출한 코루틴을 JobHandle.Wait()로 대기
    std::atomic<int> value = 0;

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::atomic<int>& v) static -> JobTask<void>
        {
            v.store(42, std::memory_order_release);
            co_return;
        }(value));

    h.Wait();
    EXPECT_EQ(value.load(std::memory_order_acquire), 42);
}

TEST_F(CoroutineTest, SubmitTask_CoAwaitHandle)
{
    // 코루틴 내에서 SubmitTask의 JobHandle을 co_await
    std::atomic<int> value = 0;

    std::promise<bool> promise;
    auto future = promise.get_future();

    JobHandle outer = JobSystem::Get().SubmitTask(
        [](std::atomic<int>& v, std::promise<bool>& p) static -> JobTask<void>
        {
            JobHandle inner = JobSystem::Get().SubmitTask(
                [](std::atomic<int>& val) static -> JobTask<void>
                {
                    val.store(99, std::memory_order_release);
                    co_return;
                }(v));

            co_await inner;
            p.set_value(v.load(std::memory_order_acquire) == 99);
            co_return;
        }(value, promise));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());
    outer.Wait();
}

TEST_F(CoroutineTest, DispatchTask_FireAndForget)
{
    // DispatchTask fire-and-forget, 부작용 확인
    std::atomic<int> value = 0;
    std::promise<void> promise;
    auto future = promise.get_future();

    JobSystem::Get().DispatchTask(
        [](std::atomic<int>& v, std::promise<void>& p) static -> JobTask<void>
        {
            v.store(77, std::memory_order_release);
            p.set_value();
            co_return;
        }(value, promise));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(value.load(std::memory_order_acquire), 77);
}

TEST_F(CoroutineTest, SubmitTask_FactoryOverload)
{
    // Factory 람다 오버로드 테스트
    std::atomic<int> value = 0;

    JobHandle h = JobSystem::Get().SubmitTask([](std::atomic<int>& v) static -> JobTask<void>
    {
        v.store(55, std::memory_order_release);
        co_return;
    }(value));

    h.Wait();
    EXPECT_EQ(value.load(std::memory_order_acquire), 55);
}

TEST_F(CoroutineTest, SubmitTask_WithSuspend)
{
    // 중간에 co_await가 있는 코루틴
    std::atomic<int> sum = 0;

    JobHandle h = JobSystem::Get().SubmitTask(
        [](std::atomic<int>& sum) static -> JobTask<void>
        {
            sum.fetch_add(10, std::memory_order_relaxed);

            co_await JobSystem::Get().Submit([&sum]
            {
                sum.fetch_add(20, std::memory_order_relaxed);
            });

            sum.fetch_add(30, std::memory_order_relaxed);
            co_return;
        }(sum));

    h.Wait();
    EXPECT_EQ(sum.load(std::memory_order_relaxed), 60);
}

// ═══════════════════════════════════════════════════════════════════
//  JobTask<T> & JobHandle<T> 통합 테스트
// ═══════════════════════════════════════════════════════════════════

TEST_F(CoroutineTest, SubmitTask_TypedReturn_Basic)
{
    JobHandle<int> h = JobSystem::Get().SubmitTask([]() static -> JobTask<int>
    {
        co_await ResumeOn{ EJobThread::Worker };
        co_return 999;
    });

    EXPECT_EQ(h.Get(), 999);
}

TEST_F(CoroutineTest, SubmitTask_TypedReturn_MoveOnlyType)
{
    // 코루틴 내부 스토리지에서 외부 JobSharedState로 이동 전용 객체가 잘 넘어오는지 검증
    JobHandle<std::unique_ptr<double>> h = JobSystem::Get().SubmitTask([]() static -> JobTask<std::unique_ptr<double>>
    {
        co_return std::make_unique<double>(3.14);
    });

    std::unique_ptr<double> result = h.Get();
    ASSERT_NE(result, nullptr);
    EXPECT_DOUBLE_EQ(*result, 3.14);
}

TEST_F(CoroutineTest, WhenAll_MixedTypedHandles)
{
    // 여러 타입의 핸들을 혼합 생성
    JobHandle<int> h_int = JobSystem::Get().Submit([] { return 777; });

    JobHandle<std::string> hStr = JobSystem::Get().SubmitTask([]() static -> JobTask<std::string>
    {
        co_return "EngineCore";
    });

    JobHandle<void> h_void = JobSystem::Get().Submit([] { /* 일반 Void 작업 */ });

    std::promise<bool> promise;
    auto future = promise.get_future();

    // 여러 타입의 핸들을 JobHandle<void>로 캐스팅하여 WhenAll에 넘기는 코루틴
    JobHandle<void> h_task = JobSystem::Get().SubmitTask(
        [](std::promise<bool>& p, JobHandle<void> a, JobHandle<void> b, JobHandle<void> c) static -> JobTask<void>
        {
            // 암시적 형변환 덕분에 타입이 달라도 완벽하게 WhenAll로 묶입니다.
            co_await WhenAll{ { a, b, c } };
            p.set_value(true);
            co_return;
        }(promise, h_int, hStr, h_void));

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_TRUE(future.get());

    // WhenAll 완료 후 각 핸들에서 고유 타입의 결과값을 안전하게 추출
    EXPECT_EQ(h_int.Get(), 777);
    EXPECT_EQ(hStr.Get(), "EngineCore");

    h_task.Wait();
}
