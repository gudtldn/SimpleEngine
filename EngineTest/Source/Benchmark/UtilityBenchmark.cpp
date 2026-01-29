#include <benchmark/benchmark.h>
#include <optional>
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Error/Expected.h"

namespace se::benchmark_test
{
// --- Optional vs std::optional ---

static void BM_SE_Optional_Construct(benchmark::State& state)
{
    for (auto _ : state)
    {
        Optional<int> opt(10);
        benchmark::DoNotOptimize(opt);
    }
}
BENCHMARK(BM_SE_Optional_Construct);

static void BM_STL_Optional_Construct(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::optional<int> opt(10);
        benchmark::DoNotOptimize(opt);
    }
}
BENCHMARK(BM_STL_Optional_Construct);

static void BM_SE_Optional_CheckAndAccess(benchmark::State& state)
{
    Optional<int> opt(10);
    for (auto _ : state)
    {
        if (opt.HasValue())
        {
            benchmark::DoNotOptimize(opt.Value());
        }
    }
}
BENCHMARK(BM_SE_Optional_CheckAndAccess);

static void BM_STL_Optional_CheckAndAccess(benchmark::State& state)
{
    std::optional<int> opt(10);
    for (auto _ : state)
    {
        if (opt.has_value())
        {
            benchmark::DoNotOptimize(opt.value());
        }
    }
}
BENCHMARK(BM_STL_Optional_CheckAndAccess);

// --- Expected ---

static void BM_SE_Expected_Construct_Value(benchmark::State& state)
{
    for (auto _ : state)
    {
        Expected<int, int> exp(10);
        benchmark::DoNotOptimize(exp);
    }
}
BENCHMARK(BM_SE_Expected_Construct_Value);

static void BM_SE_Expected_Construct_Error(benchmark::State& state)
{
    for (auto _ : state)
    {
        Expected<int, int> exp(Unexpected<int>(10));
        benchmark::DoNotOptimize(exp);
    }
}
BENCHMARK(BM_SE_Expected_Construct_Error);

static void BM_SE_Expected_CheckAndAccess(benchmark::State& state)
{
    Expected<int, int> exp(10);
    for (auto _ : state)
    {
        if (exp.HasValue())
        {
            benchmark::DoNotOptimize(exp.Value());
        }
    }
}
BENCHMARK(BM_SE_Expected_CheckAndAccess);
}
