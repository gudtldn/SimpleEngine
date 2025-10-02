#include "doctest.h"
#include "tracy/Tracy.hpp"

import std;
import SE.Prelude;
import SE.Components;


TEST_SUITE("SimpleEngine.Core:ECS")
{
using namespace se::core;
using namespace se::core::ecs;

World world;


TEST_CASE("ECS System Test")
{
    world.CreateEntity()
         .AddComponent<TransformComponent>()
         .AddComponent<MeshHandleComponent>();

    world.AddSystem([](Query<TransformComponent&, With<>, Without<>> query)
    {
    });
}
}
