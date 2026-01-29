#include <benchmark/benchmark.h>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Container/Set.h"

namespace se::benchmark_test
{
    // --- HashMap vs std::unordered_map ---

    static void BM_SE_HashMap_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            HashMap<int, int> map;
            for (int i = 0; i < state.range(0); ++i)
            {
                map.Insert(i, i);
            }
            benchmark::DoNotOptimize(map);
        }
    }
    BENCHMARK(BM_SE_HashMap_Insert)->Range(8, 8 << 10);

    static void BM_STL_UnorderedMap_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            std::unordered_map<int, int> map;
            for (int i = 0; i < state.range(0); ++i)
            {
                map.insert({i, i});
            }
            benchmark::DoNotOptimize(map);
        }
    }
    BENCHMARK(BM_STL_UnorderedMap_Insert)->Range(8, 8 << 10);

    static void BM_SE_HashMap_Find(benchmark::State& state)
    {
        HashMap<int, int> map;
        int size = state.range(0);
        for (int i = 0; i < size; ++i)
        {
            map.Insert(i, i);
        }
        
        std::mt19937 gen(1234);
        std::uniform_int_distribution<int> dist(0, size * 2);

        for (auto _ : state)
        {
            int key = dist(gen);
            auto result = map.Find(key);
            benchmark::DoNotOptimize(result);
        }
    }
    BENCHMARK(BM_SE_HashMap_Find)->Range(8, 8 << 10);

    static void BM_STL_UnorderedMap_Find(benchmark::State& state)
    {
        std::unordered_map<int, int> map;
        int size = state.range(0);
        for (int i = 0; i < size; ++i)
        {
            map.insert({i, i});
        }
        
        std::mt19937 gen(1234);
        std::uniform_int_distribution<int> dist(0, size * 2);

        for (auto _ : state)
        {
            int key = dist(gen);
            auto result = map.find(key);
            benchmark::DoNotOptimize(result);
        }
    }
    BENCHMARK(BM_STL_UnorderedMap_Find)->Range(8, 8 << 10);

    // --- HashSet vs std::unordered_set ---

    static void BM_SE_HashSet_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            HashSet<int> set;
            for (int i = 0; i < state.range(0); ++i)
            {
                set.Insert(i);
            }
            benchmark::DoNotOptimize(set);
        }
    }
    BENCHMARK(BM_SE_HashSet_Insert)->Range(8, 8 << 10);

    static void BM_STL_UnorderedSet_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            std::unordered_set<int> set;
            for (int i = 0; i < state.range(0); ++i)
            {
                set.insert(i);
            }
            benchmark::DoNotOptimize(set);
        }
    }
    BENCHMARK(BM_STL_UnorderedSet_Insert)->Range(8, 8 << 10);

    // --- Map vs std::map ---

    static void BM_SE_Map_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            Map<int, int> map;
            for (int i = 0; i < state.range(0); ++i)
            {
                map.Insert(i, i);
            }
            benchmark::DoNotOptimize(map);
        }
    }
    BENCHMARK(BM_SE_Map_Insert)->Range(8, 8 << 10);

    static void BM_STL_Map_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            std::map<int, int> map;
            for (int i = 0; i < state.range(0); ++i)
            {
                map.insert({i, i});
            }
            benchmark::DoNotOptimize(map);
        }
    }
    BENCHMARK(BM_STL_Map_Insert)->Range(8, 8 << 10);

    // --- Set vs std::set ---

    static void BM_SE_Set_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            Set<int> set;
            for (int i = 0; i < state.range(0); ++i)
            {
                set.Insert(i);
            }
            benchmark::DoNotOptimize(set);
        }
    }
    BENCHMARK(BM_SE_Set_Insert)->Range(8, 8 << 10);

    static void BM_STL_Set_Insert(benchmark::State& state)
    {
        for (auto _ : state)
        {
            std::set<int> set;
            for (int i = 0; i < state.range(0); ++i)
            {
                set.insert(i);
            }
            benchmark::DoNotOptimize(set);
        }
    }
    BENCHMARK(BM_STL_Set_Insert)->Range(8, 8 << 10);
}