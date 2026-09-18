#include <benchmark/benchmark.h>
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Cast.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"

namespace se::benchmark_test
{
// 벤치마크를 위한 상속 계층 구조
class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) BenchBase
{
    SE_CLASS_V1(BenchBase)

public:
    SE_RTTI_ROOT()

    virtual ~BenchBase() = default;
};

class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) BenchLevel1 : public BenchBase
{
    SE_CLASS_V1(BenchLevel1, BenchBase)

public:
    SE_RTTI(BenchLevel1)
};

class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) BenchLevel2 : public BenchLevel1
{
    SE_CLASS_V1(BenchLevel2, BenchLevel1)

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

class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) BenchImplementer : public BenchBase, public IBenchInterface
{
    SE_CLASS_V1(BenchImplementer, BenchBase)

public:
    SE_RTTI(BenchImplementer)

    virtual void BenchFunc() override {}
};

class BenchOther
{
    SE_CLASS_V1(BenchOther)
public:
    virtual ~BenchOther() = default;
};

SE_BEGIN_REFLECT_V1(BenchBase, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(BenchBase)

SE_BEGIN_REFLECT_V1(BenchLevel1, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(BenchLevel1)

SE_BEGIN_REFLECT_V1(BenchLevel2, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(BenchLevel2)

SE_BEGIN_REFLECT_V1(BenchImplementer, meta::Reflect, meta::Hidden, meta::Transient)
    SE_REFLECT_INTERFACE_V1(IBenchInterface)
SE_END_REFLECT_V1(BenchImplementer)

SE_BEGIN_REFLECT_V1(BenchOther, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(BenchOther)
} // namespace se::benchmark_test

// 신규 리플렉션 등록입니다. 위의 레거시 등록과 공존하며, 자체 RTTI의 base 체인을 만듭니다.
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

static void BM_SE_Cast_Success(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast_v1<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_Cast_Success);

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

static void BM_SE_Cast_Interface(benchmark::State& state)
{
    BenchImplementer implementer;
    BenchBase* base = &implementer;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast_v1<IBenchInterface>(base));
    }
}
BENCHMARK(BM_SE_Cast_Interface);

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

static void BM_SE_ExactCast_Success(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::ExactCast_v1<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_ExactCast_Success);

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

static void BM_SE_Cast_Failure(benchmark::State& state)
{
    BenchLevel1 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::Cast_v1<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_Cast_Failure);

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

static void BM_SE_IsA(benchmark::State& state)
{
    BenchLevel2 derived;
    BenchBase* base = &derived;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(se::IsA_v1<BenchLevel2>(base));
    }
}
BENCHMARK(BM_SE_IsA);

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
