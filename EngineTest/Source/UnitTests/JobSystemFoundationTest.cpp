#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <future>
#include <numeric>
#include <thread>
#include <vector>

#include "SimpleEngine/Core/Concurrency/JobAllocator.h"
#include "SimpleEngine/Core/Concurrency/JobHandle.h"
#include "SimpleEngine/Core/Concurrency/MpscTaskLinkedQueue.h"
#include "SimpleEngine/Core/Concurrency/WorkStealingDeque.h"

using namespace se;
using namespace std::chrono_literals;


// ═══════════════════════════════════════════════════════════════════
//  WorkStealingDeque Tests
// ═══════════════════════════════════════════════════════════════════

class WorkStealingDequeTest : public ::testing::Test {};

TEST_F(WorkStealingDequeTest, InitiallyEmpty)
{
    WorkStealingDeque<int> deque;
    EXPECT_TRUE(deque.IsEmpty());
    EXPECT_EQ(deque.ApproxSize(), 0);
}

TEST_F(WorkStealingDequeTest, PushPopSingleItem)
{
    WorkStealingDeque<int> deque;
    deque.Push(42);

    EXPECT_FALSE(deque.IsEmpty());
    EXPECT_EQ(deque.ApproxSize(), 1);

    auto result = deque.Pop();
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(*result, 42);
    EXPECT_TRUE(deque.IsEmpty());
}

TEST_F(WorkStealingDequeTest, PushPopLIFO)
{
    WorkStealingDeque<int> deque;
    deque.Push(1);
    deque.Push(2);
    deque.Push(3);

    // Pop은 LIFO — 마지막에 넣은 것이 먼저 나온다
    EXPECT_EQ(*deque.Pop(), 3);
    EXPECT_EQ(*deque.Pop(), 2);
    EXPECT_EQ(*deque.Pop(), 1);
    EXPECT_FALSE(deque.Pop().HasValue());
}

TEST_F(WorkStealingDequeTest, StealFIFO)
{
    WorkStealingDeque<int> deque;
    deque.Push(1);
    deque.Push(2);
    deque.Push(3);

    // Steal은 FIFO — 처음 넣은 것이 먼저 훔쳐진다
    EXPECT_EQ(*deque.Steal(), 1);
    EXPECT_EQ(*deque.Steal(), 2);
    EXPECT_EQ(*deque.Steal(), 3);
    EXPECT_FALSE(deque.Steal().HasValue());
}

TEST_F(WorkStealingDequeTest, ResizeOnCapacityExceed)
{
    // 초기 capacity: 2^3 = 8
    WorkStealingDeque<int> deque(3);

    // 8개 이상 Push하면 리사이즈 발생
    for (int i = 0; i < 20; ++i)
    {
        deque.Push(i);
    }

    EXPECT_EQ(deque.ApproxSize(), 20);

    // 모두 Pop되는지 확인
    for (int i = 19; i >= 0; --i)
    {
        auto val = deque.Pop();
        ASSERT_TRUE(val.HasValue());
        EXPECT_EQ(*val, i);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  MpscTaskLinkedQueue Tests
// ═══════════════════════════════════════════════════════════════════

class MpscQueueTest : public ::testing::Test {};

TEST_F(MpscQueueTest, InitiallyEmpty)
{
    MpscTaskLinkedQueue queue;
    EXPECT_TRUE(queue.IsEmpty());
    EXPECT_EQ(queue.Drain(), 0u);
}

TEST_F(MpscQueueTest, PushAndDrain)
{
    MpscTaskLinkedQueue queue;
    int counter = 0;

    queue.Push([&counter] { counter += 1; });
    queue.Push([&counter] { counter += 10; });
    queue.Push([&counter] { counter += 100; });

    EXPECT_FALSE(queue.IsEmpty());

    u32 executed = queue.Drain();
    EXPECT_EQ(executed, 3u);
    EXPECT_EQ(counter, 111);
    EXPECT_TRUE(queue.IsEmpty());
}

TEST_F(MpscQueueTest, DrainExecutesInOrder)
{
    MpscTaskLinkedQueue queue;
    std::vector<int> order;

    queue.Push([&order] { order.push_back(1); });
    queue.Push([&order] { order.push_back(2); });
    queue.Push([&order] { order.push_back(3); });

    queue.Drain();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// ═══════════════════════════════════════════════════════════════════
//  JobAllocator Tests
// ═══════════════════════════════════════════════════════════════════

class JobAllocatorTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        JobAllocator::Shutdown();
    }
};

TEST_F(JobAllocatorTest, AllocateAllSizeClasses)
{
    // 각 Size Class에 맞는 크기로 할당
    // Class 0: usable=56, Class 1: usable=120, Class 2: usable=248, Class 3: usable=504
    void* p0 = JobAllocator::Allocate(48);
    void* p1 = JobAllocator::Allocate(100);
    void* p2 = JobAllocator::Allocate(200);
    void* p3 = JobAllocator::Allocate(500);

    ASSERT_NE(p0, nullptr);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);

    // 서로 다른 메모리여야 함
    EXPECT_NE(p0, p1);
    EXPECT_NE(p1, p2);
    EXPECT_NE(p2, p3);

    // 메모리 접근 테스트
    std::memset(p0, 0xAA, 48);
    std::memset(p1, 0xBB, 100);
    std::memset(p2, 0xCC, 200);
    std::memset(p3, 0xDD, 500);

    JobAllocator::Free(p3);
    JobAllocator::Free(p2);
    JobAllocator::Free(p1);
    JobAllocator::Free(p0);
}

TEST_F(JobAllocatorTest, RecyclingFromFreeList)
{
    // 할당 후 해제하면 같은 메모리가 재사용되어야 함
    void* first = JobAllocator::Allocate(32);
    JobAllocator::Free(first);

    void* second = JobAllocator::Allocate(32);
    EXPECT_EQ(first, second) << "Freed block must be reused";

    JobAllocator::Free(second);
}

TEST_F(JobAllocatorTest, OversizedAllocation)
{
    // 512B 초과 -> OS 직접 할당
    void* large = JobAllocator::Allocate(1024);
    ASSERT_NE(large, nullptr);

    std::memset(large, 0xFF, 1024);

    JobAllocator::Free(large);
}

TEST_F(JobAllocatorTest, FreeNull)
{
    // nullptr Free는 안전하게 무시
    JobAllocator::Free(nullptr);
}

TEST_F(JobAllocatorTest, EvictionToGlobalPool)
{
    // MAX_CACHED_BLOCKS(128)을 초과하면 절반이 GlobalPool로 반환되어야 한다
    // 200개를 할당하고 전부 해제하면 TLS가 넘쳐서 Eviction 발생
    constexpr int NUM_BLOCKS = 200;
    std::vector<void*> blocks(NUM_BLOCKS);

    for (int i = 0; i < NUM_BLOCKS; ++i)
    {
        blocks[i] = JobAllocator::Allocate(32);
        ASSERT_NE(blocks[i], nullptr);
    }

    // 전부 해제 -> TLS counts가 200이 되면서 Eviction 다수 발생
    for (void* ptr : blocks)
    {
        JobAllocator::Free(ptr);
    }

    // Eviction 이후에도 재할당이 정상 작동해야 함
    for (int i = 0; i < NUM_BLOCKS; ++i)
    {
        void* p = JobAllocator::Allocate(32);
        ASSERT_NE(p, nullptr);
        std::memset(p, 0xCD, 32);
        JobAllocator::Free(p);
    }
}

TEST_F(JobAllocatorTest, StealFromGlobalWhenTLSEmpty)
{
    // 다른 스레드에서 많은 블록을 해제 -> GlobalPool에 쌓임
    // 메인 스레드 TLS가 비었을 때 GlobalPool에서 가져와야 함
    constexpr int NUM_BLOCKS = 256;
    std::vector<void*> blocks(NUM_BLOCKS);

    // 메인 스레드에서 할당
    for (int i = 0; i < NUM_BLOCKS; ++i)
    {
        blocks[i] = JobAllocator::Allocate(32);
    }

    // Worker 스레드에서 전부 해제 -> Worker TLS 넘침 -> GlobalPool로 반환
    std::thread worker([&blocks]
    {
        for (void* ptr : blocks)
        {
            JobAllocator::Free(ptr);
        }
    });
    worker.join();

    // 메인 스레드 TLS는 비어있으므로 GlobalPool에서 Steal
    for (int i = 0; i < NUM_BLOCKS; ++i)
    {
        void* p = JobAllocator::Allocate(32);
        ASSERT_NE(p, nullptr);
        std::memset(p, 0xEE, 32);
        JobAllocator::Free(p);
    }
}


// ═══════════════════════════════════════════════════════════════════
//  JobCounter Tests
// ═══════════════════════════════════════════════════════════════════

class JobCounterTest : public ::testing::Test {};

TEST_F(JobCounterTest, InitialCountZero_IsImmediatelyComplete)
{
    JobCounter counter(0);
    EXPECT_TRUE(counter.IsComplete());
    EXPECT_EQ(counter.GetCount(), 0u);
}

TEST_F(JobCounterTest, DecrementToZero)
{
    JobCounter counter(3);
    EXPECT_FALSE(counter.IsComplete());

    counter.Decrement();
    EXPECT_FALSE(counter.IsComplete());
    EXPECT_EQ(counter.GetCount(), 2u);

    counter.Decrement();
    EXPECT_FALSE(counter.IsComplete());

    counter.Decrement();
    EXPECT_TRUE(counter.IsComplete());
    EXPECT_EQ(counter.GetCount(), 0u);
}

TEST_F(JobCounterTest, CallbackWaiterNotifiedOnComplete)
{
    auto counter = std::make_shared<JobCounter>(2);

    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();

    std::ignore = counter->AddWaiter([&promise]
    {
        promise.set_value(true);
    });

    counter->Decrement();
    EXPECT_EQ(future.wait_for(10ms), std::future_status::timeout)
        << "Callback invoked before counter reached zero";

    counter->Decrement();
    auto status = future.wait_for(1s);
    ASSERT_EQ(status, std::future_status::ready);
    EXPECT_TRUE(future.get());
}

TEST_F(JobCounterTest, AddWaiterAfterComplete_ReturnsCallback)
{
    JobCounter counter(1);
    counter.Decrement();
    ASSERT_TRUE(counter.IsComplete());

    bool called = false;
    auto returned = counter.AddWaiter([&called] { called = true; });

    EXPECT_TRUE(returned.HasValue()) << "Already-completed counter must return the callback";
    EXPECT_FALSE(called) << "Callback must be returned, not auto-invoked";

    // 호출자가 직접 실행
    if (returned.HasValue())
    {
        (*returned)();
    }
    EXPECT_TRUE(called);
}

TEST_F(JobCounterTest, MultipleWaitersAllNotified)
{
    auto counter = std::make_shared<JobCounter>(1);
    constexpr int NUM_WAITERS = 10;

    std::atomic<int> notified_count{0};
    for (int i = 0; i < NUM_WAITERS; ++i)
    {
        std::ignore = counter->AddWaiter([&notified_count]
        {
            notified_count.fetch_add(1, std::memory_order_relaxed);
        });
    }

    counter->Decrement();

    EXPECT_EQ(notified_count.load(), NUM_WAITERS);
}

// ═══════════════════════════════════════════════════════════════════
//  JobHandle Tests
// ═══════════════════════════════════════════════════════════════════

class JobHandleTest : public ::testing::Test {};

TEST_F(JobHandleTest, DefaultHandleIsComplete)
{
    JobHandle<void> handle;
    EXPECT_TRUE(handle.IsComplete());
    EXPECT_FALSE(static_cast<bool>(handle));
}

TEST_F(JobHandleTest, CreateAndComplete)
{
    JobHandle handle = JobHandle<void>::Create(1);
    EXPECT_TRUE(static_cast<bool>(handle));
    EXPECT_FALSE(handle.IsComplete());

    handle.GetCounter()->Decrement();
    EXPECT_TRUE(handle.IsComplete());
}

TEST_F(JobHandleTest, WaitBlocksUntilComplete)
{
    JobHandle handle = JobHandle<void>::Create(1);

    std::thread worker([&handle]
    {
        std::this_thread::sleep_for(50ms);
        handle.GetCounter()->Decrement();
    });

    handle.Wait();

    EXPECT_TRUE(handle.IsComplete());
    worker.join();
}
