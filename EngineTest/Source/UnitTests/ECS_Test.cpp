#include "gtest/gtest.h"

#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/QueryData.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/MeshHandleComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"

using namespace se;
using namespace se::core;
using namespace se::ecs;
using namespace se::ecs::schedule;

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
    world.AddSystem<PreUpdate>([&]
    {
        execution_log.emplace_back(u8"PreUpdate");
    });

    world.AddSystem<Update>([&]
    {
        execution_log.emplace_back(u8"Update");
    });

    world.AddSystem<PostUpdate>([&]
    {
        execution_log.emplace_back(u8"PostUpdate");
    });

    // Run schedules in a specific order
    world.RunSchedule<PreUpdate>();
    world.RunSchedule<Update>();
    world.RunSchedule<PostUpdate>();

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
    world.AddSystem<PreUpdate>([](Query<TestValueComponent&> query)
    {
        for (auto [val] : query)
        {
            val.value += 5; // 10 + 5 = 15
        }
    });

    // System to run in Update to multiply the value
    world.AddSystem<Update>([](Query<TestValueComponent&, With<TestTagComponent>> query)
    {
        for (auto [val] : query)
        {
            val.value *= 2; // 15 * 2 = 30
        }
    });

    // System to run in PostUpdate on entities without the tag (should not run)
    world.AddSystem<PostUpdate>([](Query<TestValueComponent&, Without<TestTagComponent>> query)
    {
        for (auto [val] : query)
        {
            val.value = 0; // This should not be executed
        }
    });


    // Run the schedules
    world.RunSchedule<PreUpdate>();
    world.RunSchedule<Update>();
    world.RunSchedule<PostUpdate>();

    // Verify the final value of the component
    auto component = world.TryGetComponent<TestValueComponent>(entity);
    ASSERT_TRUE(component.HasValue());
    EXPECT_EQ(component->value, 30);

    // Verify that removing a component works with queries
    world.RemoveComponent<TestTagComponent>(entity);

    // This system should now run and set the value to 0
    world.RunSchedule<PostUpdate>();

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
         .AddComponent<MeshHandleComponent>();

    world.SpawnEntity(
        TransformComponent{
            .rotation = { 0.0, 0.0, 0.0, 1.0 },
            .position = { 1.0, 2.0, 3.0 },
            .scale = { 1.0, 1.0, 1.0 },
        },
        MeshHandleComponent{
            .mesh_id = asset::AssetId{ Guid::NewGuid() }
        }
    );

    // These AddSystem calls are primarily for compile-time validation of different parameter types.
    // They don't need to be executed to be valuable.
    world.AddSystem<Update>([](
        [[maybe_unused]] Query<TransformComponent&, With<>, Without<>> query1,
        [[maybe_unused]] Query<TransformComponent&, With<MeshHandleComponent>, Without<>> query2,
        [[maybe_unused]] Query<Entity, With<>, Without<>> query3
    )
        {
            constexpr usize query1_size = sizeof(query1);
            constexpr usize query2_size = sizeof(query2);
            constexpr usize query3_size = sizeof(query3);

            static_assert(query1_size == query2_size || query1_size != query3_size);
            SUCCEED() << "System with multiple queries compiled and ran successfully.";
        });

    world.AddSystem<Update>([](World* w)
    {
        ASSERT_NE(w, nullptr) << "System received a null World pointer.";
    });

    world.AddSystem<Update>([](Query<Entity> query)
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

    world.RunSchedule<Update>();
}

TEST_F(ECSTest, ECSSystemWithOptionalComponents)
{
    World world;

    // Create entities
    auto entity_with_component = world.SpawnEntity()
                                      .AddComponent<TestValueComponent>({ .value = 100 });

    auto entity_without_component = world.SpawnEntity();

    // System that uses Optional to modify a component if it exists
    world.AddSystem<Update>([](Query<Optional<TestValueComponent&>> query)
    {
        for (const auto& [opt_val] : query)
        {
            if (opt_val.HasValue())
            {
                opt_val->value = 200;
            }
        }
    });

    // Run the schedule
    world.RunSchedule<Update>();

    // Verify the component on the first entity was modified
    auto component = world.TryGetComponent<TestValueComponent>(entity_with_component);
    ASSERT_TRUE(component.HasValue());
    EXPECT_EQ(component->value, 200);

    // Verify the second entity still does not have the component
    auto component2 = world.TryGetComponent<TestValueComponent>(entity_without_component);
    EXPECT_FALSE(component2.HasValue());

    // System that adds the component if it's missing
    world.AddSystem<PostUpdate>([](Query<Entity, Optional<TestValueComponent&>> query, World* in_world)
    {
        for (const auto& [entity, opt_val] : query)
        {
            if (!opt_val.HasValue())
            {
                in_world->AddComponent<TestValueComponent>(entity, { .value = 50 });
            }
        }
    });

    // Run the second schedule
    world.RunSchedule<PostUpdate>();

    // Verify the second entity now has the component with the correct value
    auto component3 = world.TryGetComponent<TestValueComponent>(entity_without_component);
    ASSERT_TRUE(component3.HasValue());
    EXPECT_EQ(component3->value, 50);
}
