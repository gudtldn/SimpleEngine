#include "doctest.h"
#include "tracy/Tracy.hpp"

import std;
import SE.Prelude;
import SE.Components;
import SE.Core;


TEST_SUITE("SimpleEngine.Core:ECS")
{
using namespace se::core;
using namespace se::core::ecs;
using namespace se::core::ecs::schedules;

// A simple component for testing
struct TestValueComponent
{
    int value;
};

// A tag component for testing queries
struct TestTagComponent
{
};

TEST_CASE("ECS Schedule and System Execution Order")
{
    World world;
    std::vector<std::string> execution_log;

    // Add systems to different schedules
    world.AddSystem<PreUpdate>([&]
    {
        execution_log.push_back("PreUpdate");
    });

    world.AddSystem<Update>([&]
    {
        execution_log.push_back("Update");
    });

    world.AddSystem<PostUpdate>([&]
    {
        execution_log.push_back("PostUpdate");
    });

    // Run schedules in a specific order
    world.RunSchedule<PreUpdate>();
    world.RunSchedule<Update>();
    world.RunSchedule<PostUpdate>();

    // Verify the execution order
    REQUIRE(execution_log.size() == 3);
    CHECK(execution_log[0] == "PreUpdate");
    CHECK(execution_log[1] == "Update");
    CHECK(execution_log[2] == "PostUpdate");
}

TEST_CASE("ECS System Component Modification and Queries")
{
    World world;

    // Create an entity with a component to be modified
    auto entity = world.CreateEntity()
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
    REQUIRE(component.HasValue());
    CHECK(component->value == 30);

    // Verify that removing a component works with queries
    world.RemoveComponent<TestTagComponent>(entity);

    // This system should now run and set the value to 0
    world.RunSchedule<PostUpdate>();

    component = world.TryGetComponent<TestValueComponent>(entity);
    REQUIRE(component.HasValue());
    CHECK(component->value == 0);
}

// Keep the user's original test case as a compile-time check
TEST_CASE("ECS System Parameter Compilation Test")
{
    World world;

    world.CreateEntity()
         .AddComponent<TransformComponent>()
         .AddComponent<MeshHandleComponent>();

    // These AddSystem calls are primarily for compile-time validation of different parameter types.
    // They don't need to be executed to be valuable.
    world.AddSystem<Update>([](
        [[maybe_unused]] Query<TransformComponent&, With<>, Without<>> query1,
        [[maybe_unused]] Query<TransformComponent&, With<MeshHandleComponent>, Without<>> query2
    )
        {
            CHECK_MESSAGE(true, "System with multiple queries compiled.");
        });

    world.AddSystem<Update>([](World* w)
    {
        CHECK_MESSAGE(w != nullptr, "System with World* parameter compiled.");
    });

    world.RunSchedule<Update>();
}
}
