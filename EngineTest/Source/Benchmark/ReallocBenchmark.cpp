#include "benchmark/benchmark.h"
#include "SimpleEngine/Core/Memory/OsMemory.h"

// This benchmark is for optimizing the Realloc logic.
// It compares the current naive implementation of OsMemory::Realloc with the standard C realloc.

namespace se
{
static void BM_OsMemoryRealloc_Naive(benchmark::State& state)
{
    const auto old_size = state.range(0);
    const auto new_size = state.range(1);
    constexpr size_t ALIGNMENT = 16;

    for ([[maybe_unused]] auto _ : state)
    {
        state.PauseTiming();
        void* p = OsMemory::Allocate(old_size, ALIGNMENT);
        std::memset(p, 0xCD, old_size);
        benchmark::DoNotOptimize(p);
        state.ResumeTiming();

        void* new_p = OsMemory::Realloc(p, new_size, ALIGNMENT);
        benchmark::DoNotOptimize(new_p);

        state.PauseTiming();
        OsMemory::Free(new_p);
        state.ResumeTiming();
    }
}

// Test growing allocation
BENCHMARK(BM_OsMemoryRealloc_Naive)->ArgNames({"old_size", "new_size"})->Args({1024, 2048});
BENCHMARK(BM_OsMemoryRealloc_Naive)->ArgNames({"old_size", "new_size"})->Args({1024 * 1024, 2 * 1024 * 1024});

// Test shrinking allocation
BENCHMARK(BM_OsMemoryRealloc_Naive)->ArgNames({"old_size", "new_size"})->Args({2048, 1024});
BENCHMARK(BM_OsMemoryRealloc_Naive)->ArgNames({"old_size", "new_size"})->Args({2 * 1024 * 1024, 1024 * 1024});


static void BM_StdRealloc(benchmark::State& state)
{
    const auto old_size = state.range(0);
    const auto new_size = state.range(1);

    for ([[maybe_unused]] auto _ : state)
    {
        state.PauseTiming();
        void* p = std::malloc(old_size);
        std::memset(p, 0xCD, old_size);
        benchmark::DoNotOptimize(p);
        state.ResumeTiming();

        void* new_p = std::realloc(p, new_size);
        benchmark::DoNotOptimize(new_p);

        state.PauseTiming();
        std::free(new_p);
        state.ResumeTiming();
    }
}

// Test growing allocation
BENCHMARK(BM_StdRealloc)->ArgNames({"old_size", "new_size"})->Args({1024, 2048});
BENCHMARK(BM_StdRealloc)->ArgNames({"old_size", "new_size"})->Args({1024 * 1024, 2 * 1024 * 1024});

// Test shrinking allocation
BENCHMARK(BM_StdRealloc)->ArgNames({"old_size", "new_size"})->Args({2048, 1024});
BENCHMARK(BM_StdRealloc)->ArgNames({"old_size", "new_size"})->Args({2 * 1024 * 1024, 1024 * 1024});

}
