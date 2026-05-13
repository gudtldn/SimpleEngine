#include "gtest/gtest.h"

#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/QueryData.h"
#include "SimpleEngine/ECS/World.h"

using namespace se;


class ECSEdgeCaseTest : public ::testing::Test {};

struct EdgeCaseValue
{
    int value = 0;
};

struct EdgeCaseTag {};

struct EdgeCaseExtra
{
    f32 data = 0.0f;
};


// Entity를 파괴한 뒤 동일 슬롯에 재생성하면 이전 핸들이 무효화되는지 검증합니다.
TEST_F(ECSEdgeCaseTest, DestroyAndRecreateInvalidatesOldHandle)
{
    World world;

    Entity old_entity = world.SpawnEntity(EdgeCaseValue{ .value = 42 });
    ASSERT_TRUE(world.IsEntityAlive(old_entity));

    world.DestroyEntity(old_entity);
    EXPECT_FALSE(world.IsEntityAlive(old_entity));

    Entity new_entity = world.SpawnEntity(EdgeCaseValue{ .value = 99 });
    EXPECT_TRUE(world.IsEntityAlive(new_entity));

    // 이전 핸들로 접근하면 실패해야 합니다.
    EXPECT_FALSE(world.IsEntityAlive(old_entity));
    EXPECT_FALSE(world.TryGetComponent<EdgeCaseValue>(old_entity).HasValue());

    // 새 핸들로는 접근 가능해야 합니다.
    auto comp = world.TryGetComponent<EdgeCaseValue>(new_entity);
    ASSERT_TRUE(comp.HasValue());
    EXPECT_EQ(comp->value, 99);
}

// 동일 엔티티에 같은 컴포넌트를 두 번 추가하면 덮어쓰기되는지 검증합니다.
TEST_F(ECSEdgeCaseTest, AddComponentOverwritesExistingValue)
{
    World world;
    Entity entity = world.SpawnEntity(EdgeCaseValue{ .value = 10 });

    world.AddComponent<EdgeCaseValue>(entity, EdgeCaseValue{ .value = 77 });

    auto comp = world.TryGetComponent<EdgeCaseValue>(entity);
    ASSERT_TRUE(comp.HasValue());
    EXPECT_EQ(comp->value, 77);
}

// 컴포넌트를 추가 → 제거 → 재추가 사이클 후 값이 올바른지 검증합니다.
TEST_F(ECSEdgeCaseTest, ComponentAddRemoveReaddCycle)
{
    World world;
    Entity entity = world.SpawnEntity();

    world.AddComponent<EdgeCaseValue>(entity, EdgeCaseValue{ .value = 1 });
    ASSERT_TRUE(world.HasComponent<EdgeCaseValue>(entity));

    world.RemoveComponent<EdgeCaseValue>(entity);
    EXPECT_FALSE(world.HasComponent<EdgeCaseValue>(entity));

    world.AddComponent<EdgeCaseValue>(entity, EdgeCaseValue{ .value = 2 });
    auto comp = world.TryGetComponent<EdgeCaseValue>(entity);
    ASSERT_TRUE(comp.HasValue());
    EXPECT_EQ(comp->value, 2);
}

// 매칭되는 엔티티가 없는 빈 월드에서 Query 순회가 0회인지 검증합니다.
TEST_F(ECSEdgeCaseTest, EmptyWorldQueryReturnsNothing)
{
    World world;

    usize count = 0;
    for ([[maybe_unused]] auto _ : world.CreateQuery<EdgeCaseValue&>())
    {
        ++count;
    }
    EXPECT_EQ(count, 0u);
}

// DestroyEntity가 해당 엔티티의 모든 컴포넌트를 제거하는지 검증합니다.
TEST_F(ECSEdgeCaseTest, DestroyEntityRemovesAllComponents)
{
    World world;
    Entity entity = world.SpawnEntity(
        EdgeCaseValue{ .value = 42 },
        EdgeCaseTag{},
        EdgeCaseExtra{ .data = 3.14f }
    );

    ASSERT_TRUE(world.HasComponent<EdgeCaseValue>(entity));
    ASSERT_TRUE(world.HasComponent<EdgeCaseTag>(entity));
    ASSERT_TRUE(world.HasComponent<EdgeCaseExtra>(entity));

    world.DestroyEntity(entity);

    EXPECT_FALSE(world.IsEntityAlive(entity));
    EXPECT_FALSE(world.HasComponent<EdgeCaseValue>(entity));
    EXPECT_FALSE(world.HasComponent<EdgeCaseTag>(entity));
    EXPECT_FALSE(world.HasComponent<EdgeCaseExtra>(entity));
}

// 대량 생성 후 절반을 파괴하고 Query가 남은 엔티티만 반환하는지 검증합니다.
TEST_F(ECSEdgeCaseTest, BulkSpawnDestroyQueryConsistency)
{
    World world;
    constexpr int COUNT = 500;
    Array<Entity> entities;

    for (int i = 0; i < COUNT; ++i)
    {
        Entity e = world.SpawnEntity(EdgeCaseValue{ .value = i });
        entities.Push(e);
    }

    // 짝수 인덱스 엔티티를 파괴합니다.
    for (int i = 0; i < COUNT; i += 2)
    {
        world.DestroyEntity(entities[i]);
    }

    int query_count = 0;
    for (auto [val] : world.CreateQuery<const EdgeCaseValue&>())
    {
        EXPECT_EQ(val.value % 2, 1) << "Even-indexed entity should have been destroyed";
        ++query_count;
    }
    EXPECT_EQ(query_count, COUNT / 2);
}

// World::Reset() 후 모든 엔티티와 컴포넌트가 초기화되는지 검증합니다.
TEST_F(ECSEdgeCaseTest, WorldResetClearsEverything)
{
    World world;

    Entity e1 = world.SpawnEntity(EdgeCaseValue{ .value = 1 });
    Entity e2 = world.SpawnEntity(EdgeCaseValue{ .value = 2 });
    ASSERT_EQ(world.GetAliveEntities().Len(), 2u);

    world.Reset();

    EXPECT_EQ(world.GetAliveEntities().Len(), 0u);
    EXPECT_FALSE(world.IsEntityAlive(e1));
    EXPECT_FALSE(world.IsEntityAlive(e2));

    Entity e3 = world.SpawnEntity(EdgeCaseValue{ .value = 3 });
    EXPECT_TRUE(world.IsEntityAlive(e3));
}

// TryResolveEntity가 파괴된 엔티티에 대해 NullOpt을 반환하는지 검증합니다.
TEST_F(ECSEdgeCaseTest, TryResolveEntityAfterDestroy)
{
    World world;

    Entity entity = world.SpawnEntity();
    u32 entity_id = entity.GetId();

    ASSERT_TRUE(world.TryResolveEntity(entity_id).HasValue());

    world.DestroyEntity(entity);

    EXPECT_FALSE(world.TryResolveEntity(entity_id).HasValue());
}

// 존재하지 않는 컴포넌트를 제거해도 크래시하지 않는지 검증합니다.
TEST_F(ECSEdgeCaseTest, RemoveNonExistentComponentIsNoOp)
{
    World world;
    Entity entity = world.SpawnEntity();

    world.RemoveComponent<EdgeCaseValue>(entity);
    EXPECT_FALSE(world.HasComponent<EdgeCaseValue>(entity));
}
