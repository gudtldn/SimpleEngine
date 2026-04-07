#include "gtest/gtest.h"

#include "SimpleEngine/ECS/Commands.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/QueryData.h"
#include "SimpleEngine/ECS/WorldContext.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"
#include "SimpleEngine/Core/Time/Time.h"

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

    // Commands 파라미터가 올바르게 주입되는지 검증합니다.
    ctx.AddSystem<UpdatePhase>([]([[maybe_unused]] Commands commands)
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

    // Optional + Commands 혼합: 컴포넌트가 없는 엔티티에 새 컴포넌트를 추가합니다.
    ctx.AddSystem<PostUpdatePhase>([](Query<Entity, Optional<TestValueComponent&>> query, Commands commands)
    {
        for (const auto& [entity, opt_val] : query)
        {
            if (!opt_val.HasValue())
            {
                commands.Entity(entity).Insert(TestValueComponent{ .value = 50 });
            }
        }
    });

    ctx.RunPhase<PostUpdatePhase>();

    auto comp_added = ctx.GetWorld().TryGetComponent<TestValueComponent>(entity_without);
    ASSERT_TRUE(comp_added.HasValue());
    EXPECT_EQ(comp_added->value, 50);
}

// const World와 읽기 전용 쿼리의 상호작용을 검증합니다.
TEST_F(ECSTest, ECSConstWorldAndReadOnlyQuery)
{
    WorldContext ctx;
    auto entity = ctx.GetWorld().SpawnEntity()
                     .AddComponent<TestValueComponent>({ .value = 42 });

    // 1. Const World 확보
    const World& const_world = ctx.GetWorld();

    // 2. 읽기 전용 쿼리 생성 및 타입 검증
    using QueryTypes = std::tuple<const TestValueComponent&, Entity>;
    auto read_only_query = traits::ApplyTypes<QueryTypes>([&]<typename... Ts>
    {
        return const_world.CreateQuery<Ts...>();
    });

    static_assert(
        traits::ApplyTypes<QueryTypes>([]<typename... Ts> -> bool
        {
            return detail::IsReadOnlyQueryPack<Ts...>;
        }),
        "TargetWorld of a query created from const World must be 'const World'."
    );

    // 3. 런타임 순회 검증
    bool found = false;
    for (auto [val, ent] : read_only_query)
    {
        EXPECT_EQ(val.value, 42);
        EXPECT_EQ(ent, entity);
        found = true;

        // val.value = 100; // - 주석을 풀면 컴파일 에러가 발생해야 정상 (ReadOnly)
    }
    EXPECT_TRUE(found);

    // 4. 단일 조회 API 검증
    auto opt_res = read_only_query.TryGetSingle();
    ASSERT_TRUE(opt_res.HasValue());
    EXPECT_EQ(std::get<0>(opt_res.Value()).value, 42);

    // 5. 시스템 바인딩 검증 (읽기 전용 Query 주입)
    ctx.AddSystem<UpdatePhase>([](Query<const TestValueComponent&> query)
    {
        EXPECT_FALSE(query.IsEmpty());
    });

    ctx.RunPhase<UpdatePhase>();
}

// Commands를 통해 시스템 내부에서 구조 변경이 지연 적용되는지 검증합니다.
TEST_F(ECSTest, ECSCommandsSpawnAndDespawn)
{
    WorldContext ctx;

    // 기존 entity를 생성
    auto target = ctx.GetWorld().SpawnEntity(TestValueComponent{ .value = 999 });

    // 시스템에서 Commands로 Spawn + Despawn
    ctx.AddSystem<UpdatePhase>([target_id = static_cast<Entity>(target)](Commands commands)
    {
        commands.Spawn(TestValueComponent{ .value = 10 });
        commands.Spawn(TestValueComponent{ .value = 20 });
        commands.Entity(target_id).Despawn();
    });

    ctx.RunPhase<UpdatePhase>();

    // target은 제거되어야 하고, 2개가 새로 생성되어야 함
    EXPECT_FALSE(ctx.GetWorld().IsEntityAlive(target));

    int count = 0;
    int sum = 0;
    for (auto [val, _] : ctx.GetWorld().CreateQuery<const TestValueComponent&, Entity>())
    {
        count++;
        sum += val.value;
    }
    EXPECT_EQ(count, 2);
    EXPECT_EQ(sum, 30);
}

// Commands::Entity().Insert / Remove를 통한 지연된 컴포넌트 추가/제거를 검증합니다.
TEST_F(ECSTest, ECSCommandsInsertAndRemoveComponent)
{
    WorldContext ctx;

    auto entity = ctx.GetWorld().SpawnEntity(TestValueComponent{ .value = 1 }, TestTagComponent{});

    ctx.AddSystem<UpdatePhase>([e = static_cast<Entity>(entity)](Commands commands)
    {
        // tag 제거, value 변경
        commands.Entity(e).Remove<TestTagComponent>().Insert(TestValueComponent{ .value = 42 });
    });

    ctx.RunPhase<UpdatePhase>();

    EXPECT_FALSE(ctx.GetWorld().HasComponent<TestTagComponent>(entity));
    EXPECT_EQ(ctx.GetWorld().GetComponent<TestValueComponent>(entity).value, 42);
}

// Commands를 통해 리소스 삽입/제거가 올바르게 지연 적용되는지 검증합니다.
TEST_F(ECSTest, ECSCommandsResourceInsertAndRemove)
{
    WorldContext ctx;

    struct GameConfig
    {
        int gravity = -10;
    };

    ctx.AddSystem<UpdatePhase>([](Commands commands)
    {
        commands.InsertResource<GameConfig>(GameConfig{ .gravity = -20 });
    });

    EXPECT_FALSE(ctx.GetWorld().HasResource<GameConfig>());
    ctx.RunPhase<UpdatePhase>();
    EXPECT_TRUE(ctx.GetWorld().HasResource<GameConfig>());
    EXPECT_EQ(ctx.GetWorld().GetResource<GameConfig>().gravity, -20);

    // 제거
    ctx.AddSystem<PostUpdatePhase>([](Commands commands)
    {
        commands.RemoveResource<GameConfig>();
    });

    ctx.RunPhase<PostUpdatePhase>();
    EXPECT_FALSE(ctx.GetWorld().HasResource<GameConfig>());
}

// RunAll()을 통해 StartupPhase가 1번만 실행되고 이후 제거되는지 검증합니다.
TEST_F(ECSTest, ECSStartupPhaseRunsOnce)
{
    WorldContext ctx;

    // RunAll에 필요한 시간 리소스 등록
    ctx.GetWorld().InsertResource<RealTime>();
    ctx.GetWorld().InsertResource<GameTime>();
    ctx.GetWorld().InsertResource<FixedTime>();

    int startup_count = 0;
    int update_count = 0;

    ctx.AddSystem<StartupPhase>([&] { ++startup_count; });
    ctx.AddSystem<UpdatePhase>([&] { ++update_count; });

    ctx.RunAll(0.016);
    EXPECT_EQ(startup_count, 1);
    EXPECT_EQ(update_count, 1);

    ctx.RunAll(0.016);
    EXPECT_EQ(startup_count, 1) << "StartupPhase must not run again.";
    EXPECT_EQ(update_count, 2);

    ctx.RunAll(0.016);
    EXPECT_EQ(startup_count, 1) << "StartupPhase must not run again.";
    EXPECT_EQ(update_count, 3);
}

// RunAll()에서 Stage 순서(Startup -> PreUpdate -> FixedUpdate -> Update -> PostUpdate)가 보장되는지 검증합니다.
TEST_F(ECSTest, ECSRunAllStageOrder)
{
    WorldContext ctx;

    ctx.GetWorld().InsertResource<RealTime>();
    ctx.GetWorld().InsertResource<GameTime>();
    ctx.GetWorld().InsertResource<FixedTime>();

    int call_index = 0;
    int startup_order = -1;
    int pre_order = -1;
    int update_order = -1;
    int post_order = -1;

    ctx.AddSystem<StartupPhase>([&] { startup_order = call_index++; });
    ctx.AddSystem<PreUpdatePhase>([&] { pre_order = call_index++; });
    ctx.AddSystem<UpdatePhase>([&] { update_order = call_index++; });
    ctx.AddSystem<PostUpdatePhase>([&] { post_order = call_index++; });

    ctx.RunAll(0.016);

    EXPECT_LT(startup_order, pre_order) << "Startup must run before PreUpdate.";
    EXPECT_LT(pre_order, update_order) << "PreUpdate must run before Update.";
    EXPECT_LT(update_order, post_order) << "Update must run before PostUpdate.";
}
