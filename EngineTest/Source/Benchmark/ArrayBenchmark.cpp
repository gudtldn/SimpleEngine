#include <benchmark/benchmark.h>
#include <vector>
#include <array>
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FixedArray.h"

namespace se::benchmark_test
{
    // --- Array vs std::vector ---

    static void BM_SE_Array_Push(benchmark::State& state)
    {
        for (auto _ : state)
        {
            Array<int> arr;
            for (int i = 0; i < state.range(0); ++i)
            {
                arr.Push(i);
            }
            benchmark::DoNotOptimize(arr);
        }
    }
    BENCHMARK(BM_SE_Array_Push)->Range(8, 8 << 10);

    static void BM_STL_Vector_PushBack(benchmark::State& state)
    {
        for (auto _ : state)
        {
            std::vector<int> vec;
            for (int i = 0; i < state.range(0); ++i)
            {
                vec.push_back(i);
            }
            benchmark::DoNotOptimize(vec);
        }
    }
    BENCHMARK(BM_STL_Vector_PushBack)->Range(8, 8 << 10);

    static void BM_SE_Array_Access(benchmark::State& state)
    {
        Array<int> arr;
        int size = state.range(0);
        arr.Resize(size);
        for (int i = 0; i < size; ++i)
        {
            arr[i] = i;
        }

        for (auto _ : state)
        {
            for (int i = 0; i < size; ++i)
            {
                benchmark::DoNotOptimize(arr[i]);
            }
        }
    }
    BENCHMARK(BM_SE_Array_Access)->Range(8, 8 << 10);

    static void BM_STL_Vector_Access(benchmark::State& state)
    {
        int size = state.range(0);
        std::vector<int> vec(size);
        for (int i = 0; i < size; ++i)
        {
            vec[i] = i;
        }

        for (auto _ : state)
        {
            for (int i = 0; i < size; ++i)
            {
                benchmark::DoNotOptimize(vec[i]);
            }
        }
    }
    BENCHMARK(BM_STL_Vector_Access)->Range(8, 8 << 10);

    // --- FixedArray vs std::array ---

    static void BM_SE_FixedArray_Access(benchmark::State& state)
    {
        FixedArray<int, 1024> arr;
        for (int i = 0; i < 1024; ++i)
        {
            arr[i] = i;
        }

        for (auto _ : state)
        {
            for (int i = 0; i < 1024; ++i)
            {
                benchmark::DoNotOptimize(arr[i]);
            }
        }
    }
    BENCHMARK(BM_SE_FixedArray_Access);

    static void BM_STL_Array_Access(benchmark::State& state)
    {
        std::array<int, 1024> arr;
        for (int i = 0; i < 1024; ++i)
        {
            arr[i] = i;
        }

        for (auto _ : state)
        {
            for (int i = 0; i < 1024; ++i)
            {
                benchmark::DoNotOptimize(arr[i]);
            }
        }
    }
    BENCHMARK(BM_STL_Array_Access);
}