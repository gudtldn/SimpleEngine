#include <benchmark/benchmark.h>
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Query.h"

namespace se::benchmark_test
{
    struct Position { float x, y, z; };
    struct Velocity { float vx, vy, vz; };
    struct Acceleration { float ax, ay, az; };

    static void BM_ECS_Query_SingleComponent(benchmark::State& state)
    {
        World world;
        const int n = static_cast<int>(state.range(0));
        for (int i = 0; i < n; ++i)
        {
            world.SpawnEntity(Position{ (float)i, (float)i, (float)i });
        }

        auto query = world.CreateQuery<Position&>();

        for (auto _ : state)
        {
            for (auto [pos] : query)
            {
                pos.x += 1.0f;
                benchmark::DoNotOptimize(pos);
            }
        }
    }
    BENCHMARK(BM_ECS_Query_SingleComponent)->Range(100, 100000);

    static void BM_ECS_Query_TwoComponents(benchmark::State& state)
    {
        World world;
        const int n = static_cast<int>(state.range(0));
        for (int i = 0; i < n; ++i)
        {
            auto entity = world.SpawnEntity(Position{ (float)i, (float)i, (float)i });
            if (i % 2 == 0)
            {
                world.AddComponent(entity, Velocity{ 1.0f, 1.0f, 1.0f });
            }
        }

        auto query = world.CreateQuery<Position&, Velocity&>();

        for (auto _ : state)
        {
            for (auto [pos, vel] : query)
            {
                pos.x += vel.vx;
                benchmark::DoNotOptimize(pos);
                benchmark::DoNotOptimize(vel);
            }
        }
    }
    BENCHMARK(BM_ECS_Query_TwoComponents)->Range(100, 100000);

    static void BM_ECS_Query_ThreeComponents(benchmark::State& state)
    {
        World world;
        const int n = static_cast<int>(state.range(0));
        for (int i = 0; i < n; ++i)
        {
            auto entity = world.SpawnEntity(Position{ (float)i, (float)i, (float)i });
            if (i % 2 == 0)
            {
                world.AddComponent(entity, Velocity{ 1.0f, 1.0f, 1.0f });
            }
            if (i % 3 == 0)
            {
                world.AddComponent(entity, Acceleration{ 0.1f, 0.1f, 0.1f });
            }
        }

        auto query = world.CreateQuery<Position&, Velocity&, Acceleration&>();

        for (auto _ : state)
        {
            for (auto [pos, vel, acc] : query)
            {
                vel.vx += acc.ax;
                pos.x += vel.vx;
                benchmark::DoNotOptimize(pos);
                benchmark::DoNotOptimize(vel);
                benchmark::DoNotOptimize(acc);
            }
        }
    }
    BENCHMARK(BM_ECS_Query_ThreeComponents)->Range(100, 100000);

    // Filter benchmarks (With/Without)
    static void BM_ECS_Query_Filter_With(benchmark::State& state)
    {
        World world;
        const int n = static_cast<int>(state.range(0));
        for (int i = 0; i < n; ++i)
        {
            auto entity = world.SpawnEntity(Position{ (float)i, (float)i, (float)i });
            if (i % 10 == 0)
            {
                world.AddComponent(entity, Velocity{ 1.0f, 1.0f, 1.0f });
            }
        }

        // Only iterate entities with Position AND Velocity, but only fetch Position
        auto query = world.CreateQuery<Position&, With<Velocity>>();

        for (auto _ : state)
        {
            for (auto [pos] : query)
            {
                pos.x += 1.0f;
                benchmark::DoNotOptimize(pos);
            }
        }
    }
    BENCHMARK(BM_ECS_Query_Filter_With)->Range(100, 100000);

    static void BM_ECS_Query_Filter_Without(benchmark::State& state)
    {
        World world;
        const int n = static_cast<int>(state.range(0));
        for (int i = 0; i < n; ++i)
        {
            auto entity = world.SpawnEntity(Position{ (float)i, (float)i, (float)i });
            if (i % 10 == 0)
            {
                world.AddComponent(entity, Velocity{ 1.0f, 1.0f, 1.0f });
            }
        }

        // Iterate entities with Position but WITHOUT Velocity
        auto query = world.CreateQuery<Position&, Without<Velocity>>();

        for (auto _ : state)
        {
            for (auto [pos] : query)
            {
                pos.x += 1.0f;
                benchmark::DoNotOptimize(pos);
            }
        }
    }
    BENCHMARK(BM_ECS_Query_Filter_Without)->Range(100, 100000);
}
