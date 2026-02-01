#include "gtest/gtest.h"

#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/QueryData.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"

using namespace se;
using namespace se::ecs;

class ECSTest : public ::testing::Test {};

// A simple component for testing
struct TestValueComponent
{
    int value;
};

// A tag component for testing queries
struct TestTagComponent
{
};

TEST_F(ECSTest, ECSScheduleAndSystemExecutionOrder)
{
    World world;
    std::vector<std::u8string> execution_log;

    // Add systems to different schedules
    world.AddSystem<PreUpdatePhase>([&]
    {
        execution_log.emplace_back(u8"PreUpdate");
    });

    world.AddSystem<UpdatePhase>([&]
    {
        execution_log.emplace_back(u8"Update");
    });

    world.AddSystem<PostUpdatePhase>([&]
    {
        execution_log.emplace_back(u8"PostUpdate");
    });

    // Run schedules in a specific order
    world.RunPhase<PreUpdatePhase>();
    world.RunPhase<UpdatePhase>();
    world.RunPhase<PostUpdatePhase>();

    // Verify the execution order
    ASSERT_EQ(execution_log.size(), 3);
    EXPECT_TRUE(execution_log[0].contains(u8"PreUpdate"));
    EXPECT_TRUE(execution_log[1].contains(u8"Update"));
    EXPECT_TRUE(execution_log[2].contains(u8"PostUpdate"));
}

TEST_F(ECSTest, ECSSystemComponentModificationAndQueries)
{
    World world;

    // Create an entity with a component to be modified
    auto entity = world.SpawnEntity()
                       .AddComponent<TestValueComponent>({ .value = 10 })
                       .AddComponent<TestTagComponent>();

    // System to run in PreUpdate to add a value
    world.AddSystem<PreUpdatePhase>([](Query<TestValueComponent&> query)
    {
        for (auto [val] : query)
        {
            val.value += 5; // 10 + 5 = 15
        }
    });

    // System to run in Update to multiply the value
    world.AddSystem<UpdatePhase>([](Query<TestValueComponent&, With<TestTagComponent>> query)
    {
        for (auto [val] : query)
        {
            val.value *= 2; // 15 * 2 = 30
        }
    });

    // System to run in PostUpdate on entities without the tag (should not run)
    world.AddSystem<PostUpdatePhase>([](Query<TestValueComponent&, Without<TestTagComponent>> query)
    {
        for (auto [val] : query)
        {
            val.value = 0; // This should not be executed
        }
    });


    // Run the schedules
    world.RunPhase<PreUpdatePhase>();
    world.RunPhase<UpdatePhase>();
    world.RunPhase<PostUpdatePhase>();

    // Verify the final value of the component
    auto component = world.TryGetComponent<TestValueComponent>(entity);
    ASSERT_TRUE(component.HasValue());
    EXPECT_EQ(component->value, 30);

    // Verify that removing a component works with queries
    world.RemoveComponent<TestTagComponent>(entity);

    // This system should now run and set the value to 0
    world.RunPhase<PostUpdatePhase>();

    component = world.TryGetComponent<TestValueComponent>(entity);
    ASSERT_TRUE(component.HasValue());
    EXPECT_EQ(component->value, 0);
}

// Keep the user's original test case as a compile-time check
TEST_F(ECSTest, ECSSystemParameterCompilationTest)
{
    World world;

    world.SpawnEntity()
         .AddComponent<TransformComponent>()
         .AddComponent<StaticMeshComponent>();

    world.SpawnEntity(
        TransformComponent{
            .rotation = { 0.0, 0.0, 0.0, 1.0 },
            .position = { 1.0, 2.0, 3.0 },
            .scale = { 1.0, 1.0, 1.0 },
        },
        StaticMeshComponent{
            .mesh_id = asset::AssetId{ Guid::NewGuid() }
        }
    );

    // These AddSystem calls are primarily for compile-time validation of different parameter types.
    // They don't need to be executed to be valuable.
    world.AddSystem<UpdatePhase>([](
        [[maybe_unused]] Query<TransformComponent&, With<>, Without<>> query1,
        [[maybe_unused]] Query<TransformComponent&, With<StaticMeshComponent>, Without<>> query2,
        [[maybe_unused]] Query<Entity, With<>, Without<>> query3
    )
        {
            constexpr usize query1_size = sizeof(query1);
            constexpr usize query2_size = sizeof(query2);
            constexpr usize query3_size = sizeof(query3);

            static_assert(query1_size == query2_size || query1_size != query3_size);
            SUCCEED() << "System with multiple queries compiled and ran successfully.";
        });

    world.AddSystem<UpdatePhase>([](World* w)
    {
        ASSERT_NE(w, nullptr) << "System received a null World pointer.";
    });

    world.AddSystem<UpdatePhase>([](Query<Entity> query)
    {
        ASSERT_TRUE(!query.IsEmpty());
        for (const auto& [entity] : query)
        {
            auto opt = query.Get(entity);
            auto [ent] = *opt;

            EXPECT_EQ(ent, entity);
        }
    });

    [[maybe_unused]] Query query = world.QueryEntities<TransformComponent>();
    [[maybe_unused]] Query query_entity = world.QueryEntities<Entity>();

    world.RunPhase<UpdatePhase>();
}

TEST_F(ECSTest, ECSSystemWithOptionalComponents)
{
    World world;

    // Create entities
    auto entity_with_component = world.SpawnEntity()
                                      .AddComponent<TestValueComponent>({ .value = 100 });

    auto entity_without_component = world.SpawnEntity();

    // System that uses Optional to modify a component if it exists
    world.AddSystem<UpdatePhase>([](Query<Optional<TestValueComponent&>> query)
    {
        for (const auto& [opt_val] : query)
        {
            if (opt_val.HasValue())
            {
                opt_val->value = 200;
            }
        }
    });

    // Run the Phase
    world.RunPhase<UpdatePhase>();

    // Verify the component on the first entity was modified
    auto component = world.TryGetComponent<TestValueComponent>(entity_with_component);
    ASSERT_TRUE(component.HasValue());
    EXPECT_EQ(component->value, 200);

    // Verify the second entity still does not have the component
    auto component2 = world.TryGetComponent<TestValueComponent>(entity_without_component);
    EXPECT_FALSE(component2.HasValue());

    // System that adds the component if it's missing
    world.AddSystem<PostUpdatePhase>([](Query<Entity, Optional<TestValueComponent&>> query, World* in_world)
    {
        for (const auto& [entity, opt_val] : query)
        {
            if (!opt_val.HasValue())
            {
                in_world->AddComponent<TestValueComponent>(entity, { .value = 50 });
            }
        }
    });

    // Run the second Phase
    world.RunPhase<PostUpdatePhase>();

    // Verify the second entity now has the component with the correct value
    auto component3 = world.TryGetComponent<TestValueComponent>(entity_without_component);
    ASSERT_TRUE(component3.HasValue());
    EXPECT_EQ(component3->value, 50);
}
