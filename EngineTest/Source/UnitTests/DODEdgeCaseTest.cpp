#include "gtest/gtest.h"

#include <algorithm>
#include <atomic>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/ECS/SparseSet.h"

using namespace se;


// ============================================================================
// SparseSet 대량 데이터 처리 Tests
// ============================================================================

class SparseSetBulkTest : public ::testing::Test {};

struct DODTransformComponent
{
    f32 x = 0.0f, y = 0.0f, z = 0.0f;
};

TEST_F(SparseSetBulkTest, BulkInsert100K)
{
    EntityManager mgr;
    SparseSet<DODTransformComponent> sparse;

    constexpr usize COUNT = 100'000;

    for (usize i = 0; i < COUNT; ++i)
    {
        Entity e = mgr.Create();
        sparse.Add(e, DODTransformComponent{
            static_cast<f32>(i),
            static_cast<f32>(i) * 2.0f,
            static_cast<f32>(i) * 3.0f
        });
    }

    EXPECT_EQ(sparse.Len(), COUNT);

    const auto& components = sparse.GetComponents();
    for (usize i = 0; i < COUNT; ++i)
    {
        EXPECT_EQ(components[i].x, static_cast<f32>(i));
    }
}

TEST_F(SparseSetBulkTest, FragmentedRemoveAndReinsert)
{
    EntityManager mgr;
    SparseSet<DODTransformComponent> sparse;

    constexpr usize COUNT = 10'000;
    Array<Entity> entities;

    for (usize i = 0; i < COUNT; ++i)
    {
        Entity e = mgr.Create();
        sparse.Add(e, DODTransformComponent{static_cast<f32>(i), 0, 0});
        entities.Push(e);
    }

    // 짝수 인덱스를 랜덤 순서로 제거하여 파편화 유발
    std::vector<usize> to_remove;
    for (usize i = 0; i < COUNT; i += 2)
    {
        to_remove.push_back(i);
    }
    std::mt19937 rng(42);
    std::ranges::shuffle(to_remove, rng);

    for (usize idx : to_remove)
    {
        sparse.Remove(entities[idx]);
    }

    EXPECT_EQ(sparse.Len(), COUNT / 2);

    // 남은 홀수 엔티티가 모두 접근 가능한지 확인
    for (usize i = 1; i < COUNT; i += 2)
    {
        auto opt = sparse.Find(entities[i]);
        ASSERT_TRUE(opt.HasValue()) << "Entity at original index " << i << " lost after fragmented remove";
        EXPECT_EQ(opt->x, static_cast<f32>(i));
    }

    // 제거된 슬롯에 새 엔티티 삽입
    for (usize i = 0; i < COUNT / 2; ++i)
    {
        Entity e = mgr.Create();
        sparse.Add(e, DODTransformComponent{-1.0f, 0, 0});
    }

    EXPECT_EQ(sparse.Len(), COUNT);
}

TEST_F(SparseSetBulkTest, SwapRemoveDataIntegrity)
{
    EntityManager mgr;
    SparseSet<DODTransformComponent> sparse;

    constexpr usize COUNT = 1000;
    Array<Entity> entities;

    for (usize i = 0; i < COUNT; ++i)
    {
        Entity e = mgr.Create();
        sparse.Add(e, DODTransformComponent{static_cast<f32>(i), 0, 0});
        entities.Push(e);
    }

    // 역순으로 절반 제거
    for (usize i = COUNT; i > COUNT / 2; --i)
    {
        sparse.Remove(entities[i - 1]);
    }

    EXPECT_EQ(sparse.Len(), COUNT / 2);

    for (usize i = 0; i < COUNT / 2; ++i)
    {
        auto opt = sparse.Find(entities[i]);
        ASSERT_TRUE(opt.HasValue());
        EXPECT_EQ(opt->x, static_cast<f32>(i));
    }
}

TEST_F(SparseSetBulkTest, IterationAfterFragmentation)
{
    EntityManager mgr;
    SparseSet<DODTransformComponent> sparse;

    constexpr usize COUNT = 5000;
    Array<Entity> entities;

    for (usize i = 0; i < COUNT; ++i)
    {
        Entity e = mgr.Create();
        sparse.Add(e, DODTransformComponent{static_cast<f32>(i), 0, 0});
        entities.Push(e);
    }

    // 3의 배수 인덱스 제거
    for (usize i = 0; i < COUNT; i += 3)
    {
        sparse.Remove(entities[i]);
    }

    f32 sum = 0;
    usize iter_count = 0;
    for (const auto& [entity, comp] : sparse)
    {
        sum += comp.x;
        ++iter_count;
    }

    usize expected_count = COUNT - ((COUNT + 2) / 3);
    EXPECT_EQ(iter_count, expected_count);
    EXPECT_EQ(sparse.Len(), expected_count);
}


// ============================================================================
// ParallelFor Edge Case Tests
// ============================================================================

class ParallelForEdgeCaseTest : public ::testing::Test
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

TEST_F(ParallelForEdgeCaseTest, LargeParallelForCorrectness)
{
    constexpr usize COUNT = 100'000;
    std::vector<std::atomic<int>> flags(COUNT);

    auto handle = system->ParallelFor(COUNT, 256, [&flags](usize index)
    {
        flags[index].fetch_add(1, std::memory_order_relaxed);
    });

    handle.Wait();

    for (usize i = 0; i < COUNT; ++i)
    {
        EXPECT_EQ(flags[i].load(), 1) << "Index " << i << " processed " << flags[i].load() << " times";
    }
}

TEST_F(ParallelForEdgeCaseTest, CacheLinePaddedAccumulation)
{
    // 캐시라인 패딩된 atomic 누적기를 사용하여 병렬 합산의 정확성을 검증합니다.
    constexpr usize COUNT = 50'000;
    constexpr usize NUM_BUCKETS = 4;

    struct alignas(SE_CACHE_LINE) PaddedCounter
    {
        std::atomic<int64_t> value{0};
    };

    std::vector<PaddedCounter> buckets(NUM_BUCKETS);

    auto handle = system->ParallelFor(COUNT, COUNT / NUM_BUCKETS, [&buckets, NUM_BUCKETS](usize index)
    {
        usize bucket = std::hash<std::thread::id>{}(std::this_thread::get_id()) % NUM_BUCKETS;
        buckets[bucket].value.fetch_add(static_cast<int64_t>(index), std::memory_order_relaxed);
    });

    handle.Wait();

    int64_t total = 0;
    for (const auto& b : buckets)
    {
        total += b.value.load();
    }

    int64_t expected = static_cast<int64_t>(COUNT - 1) * COUNT / 2;
    EXPECT_EQ(total, expected);
}

TEST_F(ParallelForEdgeCaseTest, BatchSizeOne)
{
    constexpr usize COUNT = 64;
    std::vector<std::atomic<int>> touched(COUNT);

    auto handle = system->ParallelFor(COUNT, 1, [&touched](usize index)
    {
        touched[index].store(1, std::memory_order_relaxed);
    });

    handle.Wait();

    for (usize i = 0; i < COUNT; ++i)
    {
        EXPECT_EQ(touched[i].load(), 1);
    }
}

TEST_F(ParallelForEdgeCaseTest, UnevenBatchDistribution)
{
    // 전체 개수가 batch_size로 나누어떨어지지 않는 경우
    constexpr usize COUNT = 1000;
    constexpr usize BATCH_SIZE = 7;

    std::atomic<usize> sum{0};

    auto handle = system->ParallelFor(COUNT, BATCH_SIZE, [&sum](usize index)
    {
        sum.fetch_add(index, std::memory_order_relaxed);
    });

    handle.Wait();

    EXPECT_EQ(sum.load(), COUNT * (COUNT - 1) / 2);
}
