#include <benchmark/benchmark.h>
#include <vector>
#include <array>
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FixedArray.h"

namespace se::benchmark_test
{

// ============================================================
// memmove 최적화 효과 측정
// 대상 연산: Reallocate(Push realloc path), RemoveAt(0), Insert(0, ...)
//
// LargePod        — trivially copyable (최적화 대상)
// LargeNonTrivial — non-trivially copyable (변화 없어야 하는 baseline)
// ============================================================

struct alignas(16) LargePod
{
    int data[8] = {};
};
static_assert(std::is_trivially_copyable_v<LargePod>);

struct LargeNonTrivial
{
    int data[8] = {};
    ~LargeNonTrivial() {}
};
static_assert(!std::is_trivially_copyable_v<LargeNonTrivial>);

// ---- 1. Reallocate path: Push without Reserve -> 1.5x 재할당 반복 ----

static void BM_Array_PushRealloc_Int(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        Array<int> arr;
        for (int i = 0; i < n; ++i)
            arr.Push(i);
        benchmark::DoNotOptimize(arr.Data());
    }
}
BENCHMARK(BM_Array_PushRealloc_Int)->Range(64, 4096);

static void BM_Array_PushRealloc_LargePod(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        Array<LargePod> arr;
        for (int i = 0; i < n; ++i)
            arr.Push(LargePod{});
        benchmark::DoNotOptimize(arr.Data());
    }
}
BENCHMARK(BM_Array_PushRealloc_LargePod)->Range(64, 4096);

static void BM_Array_PushRealloc_LargeNonTrivial(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        Array<LargeNonTrivial> arr;
        for (int i = 0; i < n; ++i)
            arr.Push(LargeNonTrivial{});
        benchmark::DoNotOptimize(arr.Data());
    }
}
BENCHMARK(BM_Array_PushRealloc_LargeNonTrivial)->Range(64, 4096);

// ---- 2. RemoveAt(0) — worst-case O(n) shift left ----

static void BM_Array_RemoveAtFront_Int(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        Array<int> arr;
        arr.Reserve(n);
        for (int i = 0; i < n; ++i) arr.Push(i);
        state.ResumeTiming();

        while (!arr.IsEmpty())
            arr.RemoveAt(0);
    }
}
BENCHMARK(BM_Array_RemoveAtFront_Int)->Range(64, 4096);

static void BM_Array_RemoveAtFront_LargePod(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        Array<LargePod> arr;
        arr.Reserve(n);
        for (int i = 0; i < n; ++i) arr.Push(LargePod{});
        state.ResumeTiming();

        while (!arr.IsEmpty())
            arr.RemoveAt(0);
    }
}
BENCHMARK(BM_Array_RemoveAtFront_LargePod)->Range(64, 4096);

static void BM_Array_RemoveAtFront_LargeNonTrivial(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        state.PauseTiming();
        Array<LargeNonTrivial> arr;
        arr.Reserve(n);
        for (int i = 0; i < n; ++i) arr.Push(LargeNonTrivial{});
        state.ResumeTiming();

        while (!arr.IsEmpty())
            arr.RemoveAt(0);
    }
}
BENCHMARK(BM_Array_RemoveAtFront_LargeNonTrivial)->Range(64, 4096);

// ---- 3. Insert(0, ...) — worst-case O(n) shift right ----

static void BM_Array_InsertFront_Int(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        Array<int> arr;
        arr.Reserve(n);
        for (int i = 0; i < n; ++i)
            arr.Insert(0, i);
        benchmark::DoNotOptimize(arr.Data());
    }
}
BENCHMARK(BM_Array_InsertFront_Int)->Range(64, 4096);

static void BM_Array_InsertFront_LargePod(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        Array<LargePod> arr;
        arr.Reserve(n);
        for (int i = 0; i < n; ++i)
            arr.Insert(0, LargePod{});
        benchmark::DoNotOptimize(arr.Data());
    }
}
BENCHMARK(BM_Array_InsertFront_LargePod)->Range(64, 4096);

static void BM_Array_InsertFront_LargeNonTrivial(benchmark::State& state)
{
    const int n = static_cast<int>(state.range(0));
    for (auto _ : state)
    {
        Array<LargeNonTrivial> arr;
        arr.Reserve(n);
        for (int i = 0; i < n; ++i)
            arr.Insert(0, LargeNonTrivial{});
        benchmark::DoNotOptimize(arr.Data());
    }
}
BENCHMARK(BM_Array_InsertFront_LargeNonTrivial)->Range(64, 4096);

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
