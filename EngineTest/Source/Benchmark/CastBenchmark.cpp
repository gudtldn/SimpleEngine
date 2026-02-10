#include <benchmark/benchmark.h>
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"

namespace se::benchmark_test
{
// 벤치마크를 위한 상속 계층 구조
class BenchBase
{
    SE_CLASS(BenchBase)
public:
    virtual ~BenchBase() = default;
};

class BenchLevel1 : public BenchBase
{
    SE_CLASS(BenchLevel1, BenchBase)
};

class BenchLevel2 : public BenchLevel1
{
    SE_CLASS(BenchLevel2, BenchLevel1)
};

// 벤치마크를 위한 인터페이스
class IBenchInterface
{
public:
    virtual ~IBenchInterface() = default;
    virtual void BenchFunc() = 0;
};

class BenchImplementer : public BenchBase, public IBenchInterface
{
    SE_CLASS(BenchImplementer, BenchBase)
public:
    void BenchFunc() override {}
};

class BenchOther
{
    SE_CLASS(BenchOther)
public:
    virtual ~BenchOther() = default;
};

SE_BEGIN_REFLECT(BenchBase)
SE_END_REFLECT(BenchBase)

SE_BEGIN_REFLECT(BenchLevel1)
SE_END_REFLECT(BenchLevel1)

SE_BEGIN_REFLECT(BenchLevel2)
SE_END_REFLECT(BenchLevel2)

SE_BEGIN_REFLECT(BenchImplementer)
    SE_REFLECT_INTERFACE(IBenchInterface)
SE_END_REFLECT(BenchImplementer)

SE_BEGIN_REFLECT(BenchOther)
SE_END_REFLECT(BenchOther)

// --- 성공하는 캐스팅 (Downcasting) ---

static void BM_DynamicCast_Success(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(dynamic_cast<BenchLevel2*>(base));
    }
}
BENCHMARK(BM_DynamicCast_Success);

static void BM_SE_Cast_Success(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_Cast_Success);

// --- 인터페이스 캐스팅 (Cross-casting) ---

static void BM_DynamicCast_Interface(benchmark::State& state)
{
    BenchImplementer implementer;
    BenchBase* base = &implementer;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(dynamic_cast<IBenchInterface*>(base));
    }
}
BENCHMARK(BM_DynamicCast_Interface);

static void BM_SE_Cast_Interface(benchmark::State& state)
{
    BenchImplementer implementer;
    BenchBase* base = &implementer;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast<IBenchInterface>(base));
    }
}
BENCHMARK(BM_SE_Cast_Interface);

// --- 성공하는 ExactCast ---

static void BM_SE_ExactCast_Success(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::ExactCast<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_ExactCast_Success);

// --- 실패하는 캐스팅 (Invalid Downcasting) ---

static void BM_DynamicCast_Failure(benchmark::State& state)
{
    BenchLevel1 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(dynamic_cast<BenchLevel2*>(base));
    }
}
BENCHMARK(BM_DynamicCast_Failure);

static void BM_SE_Cast_Failure(benchmark::State& state)
{
    BenchLevel1 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_Cast_Failure);

// --- IsA 성능 측정 ---

static void BM_SE_IsA(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::IsA<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_IsA);

} // namespace se::benchmark_test
