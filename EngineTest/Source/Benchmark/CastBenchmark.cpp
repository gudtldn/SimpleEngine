#include <benchmark/benchmark.h>
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"

namespace se::benchmark_test
{
// 벤치마크를 위한 상속 계층 구조
class BenchBase
{
public:
    SE_RTTI_ROOT()

    virtual ~BenchBase() = default;
};

class BenchLevel1 : public BenchBase
{
public:
    SE_RTTI(BenchLevel1)
};

class BenchLevel2 : public BenchLevel1
{
public:
    SE_RTTI(BenchLevel2)
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
public:
    SE_RTTI(BenchImplementer)

    virtual void BenchFunc() override {}
};
} // namespace se::benchmark_test

// 자체 RTTI의 base 체인을 만드는 등록입니다.
SE_DECLARE_REFLECTION(se::benchmark_test::BenchBase)
SE_DECLARE_REFLECTION(se::benchmark_test::BenchLevel1)
SE_DECLARE_REFLECTION(se::benchmark_test::BenchLevel2)
SE_DECLARE_REFLECTION(se::benchmark_test::IBenchInterface)
SE_DECLARE_REFLECTION(se::benchmark_test::BenchImplementer)

SE_REFLECT_BEGIN(se::benchmark_test::BenchBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::benchmark_test::BenchLevel1)
    SE_BASE(se::benchmark_test::BenchBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::benchmark_test::BenchLevel2)
    SE_BASE(se::benchmark_test::BenchLevel1)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::benchmark_test::IBenchInterface)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::benchmark_test::BenchImplementer)
    SE_BASE(se::benchmark_test::BenchBase)
    SE_BASE(se::benchmark_test::IBenchInterface)
SE_REFLECT_END()

namespace se::benchmark_test
{
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

static void BM_Rtti_Cast_Success(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast<BenchLevel2>(base));
    }
}
BENCHMARK(BM_Rtti_Cast_Success);

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

static void BM_Rtti_Cast_Interface(benchmark::State& state)
{
    BenchImplementer implementer;
    BenchBase* base = &implementer;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast<IBenchInterface>(base));
    }
}
BENCHMARK(BM_Rtti_Cast_Interface);

// --- 성공하는 ExactCast ---

static void BM_Rtti_ExactCast_Success(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::ExactCast<BenchLevel2>(base));
    }
}
BENCHMARK(BM_Rtti_ExactCast_Success);

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

static void BM_Rtti_Cast_Failure(benchmark::State& state)
{
    BenchLevel1 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast<BenchLevel2>(base));
    }
}
BENCHMARK(BM_Rtti_Cast_Failure);

// --- IsA 성능 측정 ---

static void BM_Rtti_IsA(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::IsA<BenchLevel2>(base));
    }
}
BENCHMARK(BM_Rtti_IsA);
} // namespace se::benchmark_test
