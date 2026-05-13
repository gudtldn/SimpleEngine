#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "SimpleEngine/Core/Concurrency/JobHandle.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Concurrency/MpscTaskLinkedQueue.h"
#include "SimpleEngine/Core/Concurrency/WorkStealingDeque.h"

using namespace se;
using namespace std::chrono_literals;


// ============================================================================
// WorkStealingDeque Race Condition Tests
// ============================================================================

class WorkStealingDequeRaceTest : public ::testing::Test {};

TEST_F(WorkStealingDequeRaceTest, ConcurrentPushAndSteal)
{
    // Owner가 Push하는 동안 Thief가 동시에 Steal합니다.
    // 모든 항목이 정확히 한 번씩 소비되어야 합니다.
    WorkStealingDeque<i32> deque;
    constexpr i32 TOTAL = 50000;
    constexpr i32 NUM_THIEVES = 4;

    std::atomic<i64> stolen_sum{0};
    std::atomic<i32> stolen_count{0};
    std::atomic<bool> producer_done{false};

    std::vector<std::thread> thieves;
    for (i32 t = 0; t < NUM_THIEVES; ++t)
    {
        thieves.emplace_back([&]
        {
            while (true)
            {
                auto val = deque.Steal();
                if (val.HasValue())
                {
                    stolen_sum.fetch_add(*val, std::memory_order_relaxed);
                    stolen_count.fetch_add(1, std::memory_order_relaxed);
                }
                else if (producer_done.load(std::memory_order_acquire))
                {
                    val = deque.Steal();
                    if (val.HasValue())
                    {
                        stolen_sum.fetch_add(*val, std::memory_order_relaxed);
                        stolen_count.fetch_add(1, std::memory_order_relaxed);
                    }
                    break;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Owner: Push + 간헐적 Pop
    i64 owner_sum = 0;
    i32 owner_count = 0;
    for (i32 i = 0; i < TOTAL; ++i)
    {
        deque.Push(i);

        if (i % 3 == 0)
        {
            auto val = deque.Pop();
            if (val.HasValue())
            {
                owner_sum += *val;
                ++owner_count;
            }
        }
    }

    while (true)
    {
        auto val = deque.Pop();
        if (!val.HasValue())
        {
            break;
        }
        owner_sum += *val;
        ++owner_count;
    }

    producer_done.store(true, std::memory_order_release);

    for (auto& t : thieves)
    {
        t.join();
    }

    const i32 total_count = owner_count + stolen_count.load();
    const i64 total_sum = owner_sum + stolen_sum.load();
    const i64 expected_sum = static_cast<i64>(TOTAL - 1) * TOTAL / 2;

    EXPECT_EQ(total_count, TOTAL);
    EXPECT_EQ(total_sum, expected_sum);
}

TEST_F(WorkStealingDequeRaceTest, SingleItemContention)
{
    // Pop과 Steal이 하나의 아이템에 경합할 때 정확히 하나만 성공해야 합니다.
    constexpr i32 ITERATIONS = 1000;
    i32 pop_wins = 0;
    i32 steal_wins = 0;
    i32 neither = 0;

    for (i32 iter = 0; iter < ITERATIONS; ++iter)
    {
        WorkStealingDeque<i32> deque;
        deque.Push(iter);

        std::atomic<bool> steal_got{false};
        std::atomic<i32> steal_value{-1};

        std::thread thief([&]
        {
            auto val = deque.Steal();
            if (val.HasValue())
            {
                steal_got.store(true, std::memory_order_relaxed);
                steal_value.store(*val, std::memory_order_relaxed);
            }
        });

        auto pop_val = deque.Pop();

        thief.join();

        bool pop_got = pop_val.HasValue();
        bool thief_got = steal_got.load();

        if (pop_got && !thief_got)
        {
            EXPECT_EQ(*pop_val, iter);
            ++pop_wins;
        }
        else if (!pop_got && thief_got)
        {
            EXPECT_EQ(steal_value.load(), iter);
            ++steal_wins;
        }
        else if (!pop_got && !thief_got)
        {
            ++neither;
        }
        else
        {
            FAIL() << "Both Pop and Steal took the same item (iter=" << iter << ")";
        }
    }

    EXPECT_EQ(neither, 0) << "Some items were lost";
    EXPECT_GT(pop_wins + steal_wins, 0);
}

TEST_F(WorkStealingDequeRaceTest, GrowUnderContention)
{
    // 작은 초기 용량에서 Push로 리사이즈가 발생하는 동안 Steal이 정상 작동해야 합니다.
    WorkStealingDeque<i32> deque(2); // 초기 capacity: 4
    constexpr i32 TOTAL = 10000;

    std::atomic<i64> stolen_sum{0};
    std::atomic<i32> stolen_count{0};
    std::atomic<bool> done{false};

    std::thread thief([&]
    {
        while (!done.load(std::memory_order_acquire) || !deque.IsEmpty())
        {
            auto val = deque.Steal();
            if (val.HasValue())
            {
                stolen_sum.fetch_add(*val, std::memory_order_relaxed);
                stolen_count.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                std::this_thread::yield();
            }
        }
    });

    i64 owner_sum = 0;
    i32 owner_count = 0;
    for (i32 i = 0; i < TOTAL; ++i)
    {
        deque.Push(i);
    }

    while (true)
    {
        auto val = deque.Pop();
        if (!val.HasValue())
        {
            break;
        }
        owner_sum += *val;
        ++owner_count;
    }

    done.store(true, std::memory_order_release);
    thief.join();

    const i64 total_sum = owner_sum + stolen_sum.load();
    const i64 expected_sum = static_cast<i64>(TOTAL - 1) * TOTAL / 2;

    EXPECT_EQ(owner_count + stolen_count.load(), TOTAL);
    EXPECT_EQ(total_sum, expected_sum);
}


// ============================================================================
// JobCounter Race Condition Tests
// ============================================================================

class JobCounterRaceTest : public ::testing::Test {};

TEST_F(JobCounterRaceTest, ConcurrentAddWaiterAndDecrement)
{
    // Waiter 등록과 Decrement가 동시에 경합해도 카운터가 정상 완료되어야 합니다.
    constexpr i32 ITERATIONS = 500;

    for (i32 iter = 0; iter < ITERATIONS; ++iter)
    {
        auto counter = std::make_shared<JobCounter>(2);
        std::atomic<i32> notified{0};

        std::thread waiter_thread([&]
        {
            std::ignore = counter->AddWaiter([&notified]
            {
                notified.fetch_add(1, std::memory_order_relaxed);
            });
        });

        std::thread dec_thread1([&] { counter->Decrement(); });
        std::thread dec_thread2([&] { counter->Decrement(); });

        waiter_thread.join();
        dec_thread1.join();
        dec_thread2.join();

        EXPECT_TRUE(counter->IsComplete());
        EXPECT_LE(notified.load(), 1);
    }
}

TEST_F(JobCounterRaceTest, ManyWaitersWithConcurrentDecrement)
{
    // 다수의 Waiter를 등록하면서 동시에 Decrement하는 스트레스 테스트
    constexpr i32 NUM_WAITERS = 16;
    constexpr i32 NUM_DECREMENTERS = 4;

    auto counter = std::make_shared<JobCounter>(NUM_DECREMENTERS);
    std::atomic<i32> notified_count{0};

    std::vector<std::thread> threads;

    for (i32 i = 0; i < NUM_WAITERS; ++i)
    {
        threads.emplace_back([&]
        {
            auto returned = counter->AddWaiter([&notified_count]
            {
                notified_count.fetch_add(1, std::memory_order_relaxed);
            });
            if (returned.HasValue())
            {
                (*returned)();
            }
        });
    }

    for (i32 i = 0; i < NUM_DECREMENTERS; ++i)
    {
        threads.emplace_back([&] { counter->Decrement(); });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_TRUE(counter->IsComplete());
    EXPECT_EQ(notified_count.load(), NUM_WAITERS);
}


// ============================================================================
// JobSystem Diamond Dependency Tests
// ============================================================================

class JobSystemDiamondTest : public ::testing::Test
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

TEST_F(JobSystemDiamondTest, DiamondDependency)
{
    // A -> B, A -> C, B+C -> D
    std::atomic<i32> order_counter{0};
    std::atomic<i32> a_order{0}, b_order{0}, c_order{0}, d_order{0};

    auto a = system->Submit([&]
    {
        a_order.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
    });

    auto b = system->Submit([&]
    {
        b_order.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
    }, {a});

    auto c = system->Submit([&]
    {
        c_order.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
    }, {a});

    auto d = system->Submit([&]
    {
        d_order.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
    }, {b, c});

    d.Wait();

    EXPECT_GT(b_order.load(), a_order.load()) << "B must execute after A";
    EXPECT_GT(c_order.load(), a_order.load()) << "C must execute after A";
    EXPECT_GT(d_order.load(), b_order.load()) << "D must execute after B";
    EXPECT_GT(d_order.load(), c_order.load()) << "D must execute after C";
}

TEST_F(JobSystemDiamondTest, DiamondDependencyStress)
{
    // Diamond 의존성을 반복 실행하여 레이스 컨디션을 검출합니다.
    constexpr i32 ITERATIONS = 200;

    for (i32 iter = 0; iter < ITERATIONS; ++iter)
    {
        std::atomic<i32> order_counter{0};
        std::atomic<i32> a_done{0}, b_done{0}, c_done{0}, d_done{0};

        auto a = system->Submit([&]
        {
            a_done.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
        });

        auto b = system->Submit([&]
        {
            b_done.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
        }, {a});

        auto c = system->Submit([&]
        {
            c_done.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
        }, {a});

        auto d = system->Submit([&]
        {
            d_done.store(order_counter.fetch_add(1) + 1, std::memory_order_relaxed);
        }, {b, c});

        d.Wait();

        EXPECT_EQ(order_counter.load(), 4) << "All 4 jobs must execute (iter=" << iter << ")";
        EXPECT_GT(d_done.load(), b_done.load()) << "D after B (iter=" << iter << ")";
        EXPECT_GT(d_done.load(), c_done.load()) << "D after C (iter=" << iter << ")";
    }
}

TEST_F(JobSystemDiamondTest, WideJoinDependency)
{
    // 다수의 독립 Job이 하나의 최종 Job에 합류하는 패턴
    constexpr i32 FAN_WIDTH = 32;
    std::atomic<i32> completed{0};

    Array<JobHandle> fan_handles;
    for (i32 i = 0; i < FAN_WIDTH; ++i)
    {
        fan_handles.Push(system->Submit([&completed]
        {
            completed.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    auto join = system->Submit([&completed, FAN_WIDTH]
    {
        EXPECT_EQ(completed.load(), FAN_WIDTH);
    }, ArrayView<const JobHandle>(fan_handles.Data(), fan_handles.Len()));

    join.Wait();

    EXPECT_EQ(completed.load(), FAN_WIDTH);
}


// ============================================================================
// MpscQueue Race Condition Tests
// ============================================================================

class MpscQueueRaceTest : public ::testing::Test {};

TEST_F(MpscQueueRaceTest, HighContentionMultiProducer)
{
    // 많은 Producer가 동시에 Push하면서 Consumer가 계속 Drain합니다.
    MpscTaskLinkedQueue queue;
    constexpr i32 NUM_PRODUCERS = 16;
    constexpr i32 ITEMS_PER_PRODUCER = 5000;
    std::atomic<i32> total_executed{0};
    std::atomic<bool> producers_done{false};

    std::thread consumer([&]
    {
        while (!producers_done.load(std::memory_order_acquire) || !queue.IsEmpty())
        {
            queue.Drain();
            std::this_thread::yield();
        }
        queue.Drain();
    });

    std::vector<std::thread> producers;
    for (i32 p = 0; p < NUM_PRODUCERS; ++p)
    {
        producers.emplace_back([&]
        {
            for (i32 i = 0; i < ITEMS_PER_PRODUCER; ++i)
            {
                queue.Push([&total_executed]
                {
                    total_executed.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    for (auto& t : producers)
    {
        t.join();
    }
    producers_done.store(true, std::memory_order_release);

    consumer.join();

    EXPECT_EQ(total_executed.load(), NUM_PRODUCERS * ITEMS_PER_PRODUCER);
}
