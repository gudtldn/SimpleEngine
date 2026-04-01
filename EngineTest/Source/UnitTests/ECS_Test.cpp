#include "gtest/gtest.h"

#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/QueryData.h"
#include "SimpleEngine/ECS/WorldContext.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"

using namespace se;

class ECSTest : public ::testing::Test
{
};

struct TestValueComponent
{
    int value;
};

struct TestTagComponent
{
};


// Phase 순서(PreUpdate -> Update -> PostUpdate)가 보장되는지 검증합니다.
TEST_F(ECSTest, ECSScheduleAndSystemExecutionOrder)
{
    WorldContext ctx;

    int call_index = 0;
    int pre_order = -1;
    int update_order = -1;
    int post_order = -1;

    ctx.AddSystem<PreUpdatePhase>([&] { pre_order = call_index++; });
    ctx.AddSystem<UpdatePhase>([&] { update_order = call_index++; });
    ctx.AddSystem<PostUpdatePhase>([&] { post_order = call_index++; });

    ctx.RunPhase<PreUpdatePhase>();
    ctx.RunPhase<UpdatePhase>();
    ctx.RunPhase<PostUpdatePhase>();

    ASSERT_EQ(call_index, 3) << "All 3 systems must run.";
    EXPECT_LT(pre_order, update_order) << "PreUpdate must run before Update.";
    EXPECT_LT(update_order, post_order) << "Update must run before PostUpdate.";
}


// With<>/Without<> 필터 조건에 따라 시스템 실행 여부가 올바르게 제어되는지 검증합니다.
TEST_F(ECSTest, ECSSystemComponentModificationAndQueries)
{
    WorldContext ctx;

    // TestTagComponent 보유: With<TestTagComponent> 매칭, Without<TestTagComponent> 미매칭
    auto entity = ctx.GetWorld().SpawnEntity()
                     .AddComponent<TestValueComponent>({ .value = 10 })
                     .AddComponent<TestTagComponent>();

    ctx.AddSystem<PreUpdatePhase>([](Query<TestValueComponent&> query)
    {
        for (auto [val] : query)
        {
            val.value += 5; // 10 -> 15
        }
    });

    // With<TestTagComponent>: Tag 보유 엔티티만 매칭
    ctx.AddSystem<UpdatePhase>([](Query<TestValueComponent&, With<TestTagComponent>> query)
    {
        for (auto [val] : query)
        {
            val.value *= 2; // 15 -> 30
        }
    });

    // Without<TestTagComponent>: Tag 미보유 엔티티만 매칭 -> 현재 실행되지 않아야 합니다.
    ctx.AddSystem<PostUpdatePhase>([](Query<TestValueComponent&, Without<TestTagComponent>> query)
    {
        for (auto [val] : query)
        {
            val.value = 0; // must not execute
        }
    });

    ctx.RunPhase<PreUpdatePhase>();
    ctx.RunPhase<UpdatePhase>();
    ctx.RunPhase<PostUpdatePhase>();

    auto component = ctx.GetWorld().TryGetComponent<TestValueComponent>(entity);
    ASSERT_TRUE(component.HasValue());
    EXPECT_EQ(component->value, 30) << "Without<> system must not run while entity has the tag.";

    // Tag 제거 후: Without<> 시스템이 매칭되어 실행되어야 합니다.
    ctx.GetWorld().RemoveComponent<TestTagComponent>(entity);
    ctx.RunPhase<PostUpdatePhase>();

    component = ctx.GetWorld().TryGetComponent<TestValueComponent>(entity);
    ASSERT_TRUE(component.HasValue());
    EXPECT_EQ(component->value, 0) << "Without<> system must run after tag is removed.";
}


// 다중 Query 파라미터 주입, With<> 필터 동작, World& 파라미터 주입을 검증합니다.
TEST_F(ECSTest, ECSQueryParameterInjection)
{
    WorldContext ctx;

    // entity_a, entity_b: TransformComponent + StaticMeshComponent 모두 보유
    ctx.GetWorld().SpawnEntity()
       .AddComponent<TransformComponent>()
       .AddComponent<StaticMeshComponent>();

    ctx.GetWorld().SpawnEntity(
        TransformComponent{
            .rotation = { 0.0, 0.0, 0.0, 1.0 },
            .position = { 1.0, 2.0, 3.0 },
            .scale = { 1.0, 1.0, 1.0 },
        },
        StaticMeshComponent{ .mesh_id = asset::AssetId{ Guid::NewGuid() } }
    );

    // entity_c: TransformComponent만 보유 (StaticMeshComponent 없음)
    ctx.GetWorld().SpawnEntity()
       .AddComponent<TransformComponent>();

    // 타입이 다른 Query 3개가 동일 시스템에 동시에 주입되고,
    // With<> 필터가 실제로 결과 집합을 좁히는지 카운트로 검증합니다.
    ctx.AddSystem<UpdatePhase>([](
        Query<TransformComponent&> query_all,
        Query<TransformComponent&, With<StaticMeshComponent>> query_with_mesh,
        Query<Entity> query_entity
    )
        {
            usize count_all = 0;
            usize count_mesh = 0;
            usize count_entity = 0;
            for ([[maybe_unused]] auto _ : query_all) ++count_all;
            for ([[maybe_unused]] auto _ : query_with_mesh) ++count_mesh;
            for ([[maybe_unused]] auto _ : query_entity) ++count_entity;

            EXPECT_EQ(count_all, 3u) << "Expected 3 entities with TransformComponent.";
            EXPECT_EQ(count_mesh, 2u) << "Expected 2 entities after With<StaticMeshComponent> filter.";
            EXPECT_EQ(count_entity, 3u) << "Expected 3 total entities.";
            EXPECT_LT(count_mesh, count_all) << "With<> filter must narrow the result set.";
        });

    // World& 파라미터가 올바르게 주입되는지 검증합니다.
    ctx.AddSystem<UpdatePhase>([](World& world)
    {
    });

    ctx.RunPhase<UpdatePhase>();
}


// Optional<T&> 파라미터로 컴포넌트 유무에 관계없이 전 엔티티를 순회할 수 있는지 검증합니다.
TEST_F(ECSTest, ECSSystemWithOptionalComponents)
{
    WorldContext ctx;

    auto entity_with = ctx.GetWorld().SpawnEntity()
                          .AddComponent<TestValueComponent>({ .value = 100 });
    auto entity_without = ctx.GetWorld().SpawnEntity(); // TestValueComponent 없음

    // Optional<T&>: 컴포넌트가 없는 엔티티도 순회하되, HasValue()로 존재 여부를 확인합니다.
    ctx.AddSystem<UpdatePhase>([](Query<Optional<TestValueComponent&>> query)
    {
        for (const auto& [opt_val] : query)
        {
            if (opt_val.HasValue())
            {
                opt_val->value = 200;
            }
        }
    });

    ctx.RunPhase<UpdatePhase>();

    auto comp_with = ctx.GetWorld().TryGetComponent<TestValueComponent>(entity_with);
    ASSERT_TRUE(comp_with.HasValue());
    EXPECT_EQ(comp_with->value, 200);

    // 컴포넌트가 없는 엔티티는 영향을 받지 않아야 합니다.
    EXPECT_FALSE(ctx.GetWorld().TryGetComponent<TestValueComponent>(entity_without).HasValue());

    // Optional + World& 혼합: 컴포넌트가 없는 엔티티에 새 컴포넌트를 추가합니다.
    ctx.AddSystem<PostUpdatePhase>([](Query<Entity, Optional<TestValueComponent&>> query, World& world)
    {
        for (const auto& [entity, opt_val] : query)
        {
            if (!opt_val.HasValue())
            {
                world.AddComponent<TestValueComponent>(entity, { .value = 50 });
            }
        }
    });

    ctx.RunPhase<PostUpdatePhase>();

    auto comp_added = ctx.GetWorld().TryGetComponent<TestValueComponent>(entity_without);
    ASSERT_TRUE(comp_added.HasValue());
    EXPECT_EQ(comp_added->value, 50);
}
