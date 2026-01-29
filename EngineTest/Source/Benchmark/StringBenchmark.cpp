#include <benchmark/benchmark.h>
#include <string>
#include "SimpleEngine/Core/Container/String.h"

namespace se::benchmark_test
{
    // --- String vs std::string ---

    static void BM_SE_String_Append(benchmark::State& state)
    {
        for (auto _ : state)
        {
            String str;
            for (int i = 0; i < state.range(0); ++i)
            {
                str.Append("a");
            }
            benchmark::DoNotOptimize(str);
        }
    }
    BENCHMARK(BM_SE_String_Append)->Range(8, 8 << 10);

    static void BM_STL_String_Append(benchmark::State& state)
    {
        for (auto _ : state)
        {
            std::string str;
            for (int i = 0; i < state.range(0); ++i)
            {
                str.append("a");
            }
            benchmark::DoNotOptimize(str);
        }
    }
    BENCHMARK(BM_STL_String_Append)->Range(8, 8 << 10);

    static void BM_SE_String_Find(benchmark::State& state)
    {
        String str;
        int size = state.range(0);
        for (int i = 0; i < size; ++i)
        {
            str.Append("a");
        }
        str.Append("b"); // Target to find

        for (auto _ : state)
        {
            auto pos = str.Find("b");
            benchmark::DoNotOptimize(pos);
        }
    }
    BENCHMARK(BM_SE_String_Find)->Range(8, 8 << 10);

    static void BM_STL_String_Find(benchmark::State& state)
    {
        std::string str;
        int size = state.range(0);
        for (int i = 0; i < size; ++i)
        {
            str.append("a");
        }
        str.append("b"); // Target to find

        for (auto _ : state)
        {
            auto pos = str.find("b");
            benchmark::DoNotOptimize(pos);
        }
    }
    BENCHMARK(BM_STL_String_Find)->Range(8, 8 << 10);
}