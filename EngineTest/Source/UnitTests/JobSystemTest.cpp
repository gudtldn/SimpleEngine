#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "SimpleEngine/Core/Concurrency/JobPayload.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"

using namespace se;
using namespace std::chrono_literals;


// ═══════════════════════════════════════════════════════════════════
//  Test Fixture: JobSystem 인스턴스 수명 관리
// ═══════════════════════════════════════════════════════════════════

class JobSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 테스트용 워커 4개
        system = std::make_unique<JobSystem>(4);
    }

    void TearDown() override
    {
        system.reset();
    }

    std::unique_ptr<JobSystem> system;
};


// ═══════════════════════════════════════════════════════════════════
//  JobPayload Tests
// ═══════════════════════════════════════════════════════════════════

class JobPayloadTest : public ::testing::Test {};

TEST_F(JobPayloadTest, SBO_SmallLambda)
{
    // 작은 캡처(16 바이트)는 SBO에 인라인으로 저장됩니다
    int value = 0;
    int* ptr = &value;

    auto* payload = JobPayload::Create(
        [ptr]() { *ptr = 42; },
        EJobPriority::Normal
    );

    EXPECT_EQ(payload->heap_block, nullptr);  // SBO 사용 확인
    EXPECT_EQ(payload->priority, EJobPriority::Normal);

    payload->Invoke();
    EXPECT_EQ(value, 42);

    delete payload;
}

TEST_F(JobPayloadTest, SBO_MaxCaptureFitsInline)
{
    // SBO 용량(48바이트) 이내의 캡처는 인라인으로 저장됩니다
    struct LargeCapture
    {
        uint64 a, b, c, d, e;  // 40 바이트
    };
    static_assert(sizeof(LargeCapture) <= JobPayload::SBO_CAPACITY);

    LargeCapture cap{1, 2, 3, 4, 5};
    uint64 result = 0;

    auto* payload = JobPayload::Create(
        [cap, &result]() { result = cap.a + cap.b + cap.c + cap.d + cap.e; },
        EJobPriority::Critical
    );

    EXPECT_EQ(payload->heap_block, nullptr);  // SBO 사용
    payload->Invoke();
    EXPECT_EQ(result, 15u);

    delete payload;
}

TEST_F(JobPayloadTest, HeapFallback_OversizedCapture)
{
    // SBO 용량을 초과하는 캡처는 JobAllocator에서 외부 블록을 할당합니다
    struct HugeCapture
    {
        uint64 data[8];  // 64 바이트 > SBO_CAPACITY(48)
    };
    static_assert(sizeof(HugeCapture) > JobPayload::SBO_CAPACITY);

    HugeCapture cap{};
    cap.data[0] = 99;

    auto* payload = JobPayload::Create(
        [cap]() { EXPECT_EQ(cap.data[0], 99u); },
        EJobPriority::Low
    );

    EXPECT_NE(payload->heap_block, nullptr);  // 외부 블록 사용
    payload->Invoke();

    delete payload;
}

TEST_F(JobPayloadTest, PriorityPreserved)
{
    auto* p1 = JobPayload::Create([]() {}, EJobPriority::Critical);
    auto* p2 = JobPayload::Create([]() {}, EJobPriority::Normal);
    auto* p3 = JobPayload::Create([]() {}, EJobPriority::Low);

    EXPECT_EQ(p1->priority, EJobPriority::Critical);
    EXPECT_EQ(p2->priority, EJobPriority::Normal);
    EXPECT_EQ(p3->priority, EJobPriority::Low);

    delete p1;
    delete p2;
    delete p3;
}


// ═══════════════════════════════════════════════════════════════════
//  JobSystem: 기본 제출 테스트
// ═══════════════════════════════════════════════════════════════════

TEST_F(JobSystemTest, SubmitSingleJob)
{
    std::atomic<int> value{0};
    auto handle = system->Submit([&value]() { value.store(42, std::memory_order_relaxed); });

    handle.Wait();
    EXPECT_EQ(value.load(), 42);
}

TEST_F(JobSystemTest, SubmitMultipleJobs)
{
    constexpr int JOB_COUNT = 100;
    std::atomic<int> counter{0};

    std::vector<JobHandle> handles;
    handles.reserve(JOB_COUNT);

    for (int i = 0; i < JOB_COUNT; ++i)
    {
        handles.push_back(system->Submit([&counter]()
        {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& h : handles)
    {
        h.Wait();
    }

    EXPECT_EQ(counter.load(), JOB_COUNT);
}

TEST_F(JobSystemTest, SubmitWithPriority)
{
    std::atomic<int> value{0};

    auto handle = system->Submit(
        [&value]() { value.store(1, std::memory_order_relaxed); },
        EJobPriority::Critical
    );

    handle.Wait();
    EXPECT_EQ(value.load(), 1);
}


// ═══════════════════════════════════════════════════════════════════
//  JobSystem: 의존성 테스트
// ═══════════════════════════════════════════════════════════════════

TEST_F(JobSystemTest, SubmitWithDependencies_AllComplete)
{
    // A, B 완료 후 C가 실행됩니다
    std::atomic<int> order_counter{0};
    std::atomic<int> a_order{0};
    std::atomic<int> b_order{0};
    std::atomic<int> c_order{0};

    auto a = system->Submit([&]()
    {
        a_order.store(order_counter.fetch_add(1, std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    });

    auto b = system->Submit([&]()
    {
        b_order.store(order_counter.fetch_add(1, std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    });

    auto c = system->Submit([&]()
    {
        c_order.store(order_counter.fetch_add(1, std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }, {a, b});

    c.Wait();

    // C는 반드시 A, B 이후에 실행되어야 합니다
    EXPECT_GT(c_order.load(), a_order.load());
    EXPECT_GT(c_order.load(), b_order.load());
}

TEST_F(JobSystemTest, SubmitWithDependencies_ChainedDeps)
{
    // A -> B -> C 순차 체인
    std::vector<int> execution_log;
    std::mutex log_mutex;

    auto a = system->Submit([&]()
    {
        std::scoped_lock lock(log_mutex);
        execution_log.push_back(1);
    });

    auto b = system->Submit([&]()
    {
        std::scoped_lock lock(log_mutex);
        execution_log.push_back(2);
    }, {a});

    auto c = system->Submit([&]()
    {
        std::scoped_lock lock(log_mutex);
        execution_log.push_back(3);
    }, {b});

    c.Wait();

    ASSERT_EQ(execution_log.size(), 3u);
    EXPECT_EQ(execution_log[0], 1);
    EXPECT_EQ(execution_log[1], 2);
    EXPECT_EQ(execution_log[2], 3);
}

TEST_F(JobSystemTest, SubmitWithDependencies_AlreadyComplete)
{
    // 이미 완료된 의존성은 즉시 스케줄링됩니다
    std::atomic<int> value{0};

    auto a = system->Submit([&value]() { value.store(10, std::memory_order_relaxed); });
    a.Wait();  // A 완료 대기

    auto b = system->Submit(
        [&value]() { value.fetch_add(5, std::memory_order_relaxed); },
        {a}
    );
    b.Wait();

    EXPECT_EQ(value.load(), 15);
}

TEST_F(JobSystemTest, SubmitWithDependencies_EmptyDeps)
{
    // 빈 의존성 목록은 즉시 실행됩니다
    std::atomic<int> value{0};

    auto handle = system->Submit(
        [&value]() { value.store(77, std::memory_order_relaxed); },
        {}
    );
    handle.Wait();

    EXPECT_EQ(value.load(), 77);
}


// ═══════════════════════════════════════════════════════════════════
//  JobSystem: ParallelFor 테스트
// ═══════════════════════════════════════════════════════════════════

TEST_F(JobSystemTest, ParallelFor_AllIndicesProcessed)
{
    constexpr usize COUNT = 256;
    std::atomic<usize> sum{0};

    auto handle = system->ParallelFor(COUNT, 32, [&sum](usize index)
    {
        sum.fetch_add(index, std::memory_order_relaxed);
    });

    handle.Wait();

    // 0 + 1 + 2 + ... + 255 = 255 * 256 / 2 = 32640
    EXPECT_EQ(sum.load(), COUNT * (COUNT - 1) / 2);
}

TEST_F(JobSystemTest, ParallelFor_SingleBatch)
{
    constexpr usize COUNT = 10;
    std::atomic<usize> counter{0};

    auto handle = system->ParallelFor(COUNT, COUNT, [&counter](usize)
    {
        counter.fetch_add(1, std::memory_order_relaxed);
    });

    handle.Wait();
    EXPECT_EQ(counter.load(), COUNT);
}

TEST_F(JobSystemTest, ParallelFor_ZeroCount)
{
    auto handle = system->ParallelFor(0, 32, [](usize)
    {
        FAIL() << "Should not be called";
    });

    // 빈 JobHandle 반환 확인
    EXPECT_TRUE(handle.IsComplete());
}


// ═══════════════════════════════════════════════════════════════════
//  JobSystem: 메인 스레드 큐 테스트
// ═══════════════════════════════════════════════════════════════════

TEST_F(JobSystemTest, SubmitMain_ExecuteOnDrain)
{
    std::atomic<int> value{0};

    system->SubmitMain([&value]() { value.store(99); });

    // Drain 전에는 실행되지 않습니다
    EXPECT_EQ(value.load(), 0);

    usize executed = system->ExecuteMainThreadJobs();

    EXPECT_EQ(executed, 1u);
    EXPECT_EQ(value.load(), 99);
}

TEST_F(JobSystemTest, SubmitMain_MultipleTasksDrained)
{
    int total = 0;

    system->SubmitMain([&total]() { total += 10; });
    system->SubmitMain([&total]() { total += 20; });
    system->SubmitMain([&total]() { total += 30; });

    usize count = system->ExecuteMainThreadJobs();

    EXPECT_EQ(count, 3u);
    EXPECT_EQ(total, 60);
}


// ═══════════════════════════════════════════════════════════════════
//  JobSystem: 스트레스 테스트
// ═══════════════════════════════════════════════════════════════════

TEST_F(JobSystemTest, StressTest_ManyJobs)
{
    constexpr int TOTAL_JOBS = 10000;
    std::atomic<int> counter{0};

    std::vector<JobHandle> handles;
    handles.reserve(TOTAL_JOBS);

    for (int i = 0; i < TOTAL_JOBS; ++i)
    {
        handles.push_back(system->Submit([&counter]()
        {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& h : handles)
    {
        h.Wait();
    }

    EXPECT_EQ(counter.load(), TOTAL_JOBS);
}

TEST_F(JobSystemTest, StressTest_MultiThreadedSubmit)
{
    constexpr int THREADS = 8;
    constexpr int JOBS_PER_THREAD = 500;
    std::atomic<int> counter{0};

    std::vector<std::jthread> submitters;
    submitters.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t)
    {
        submitters.emplace_back([this, &counter]()
        {
            for (int i = 0; i < JOBS_PER_THREAD; ++i)
            {
                auto handle = system->Submit([&counter]()
                {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
                handle.Wait();
            }
        });
    }

    // jthread 소멸 시 자동 join
    submitters.clear();

    EXPECT_EQ(counter.load(), THREADS * JOBS_PER_THREAD);
}

TEST_F(JobSystemTest, WorkerCount)
{
    EXPECT_EQ(system->GetWorkerCount(), 4u);
}
