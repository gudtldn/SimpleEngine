#include <benchmark/benchmark.h>
#include <random>

#include "SimpleEngine/Core/HAL/CpuFeature.h"
#include "SimpleEngine/Core/Math/MathSimd.h"
#include "SimpleEngine/Core/Math/Matrix.h"

// Accessing internal detail for benchmark comparison
namespace se::math::simd::detail
{
    template <traits::FloatingType T>
    void Matrix4x4MultiplyGeneric(const T* lhs, const T* rhs, T* result);

#if SE_ARCH_X86_FAMILY
    void Matrix4x4MultiplySSEImpl(const f32* lhs, const f32* rhs, f32* result);
    void Matrix4x4MultiplyFMAImpl(const f32* lhs, const f32* rhs, f32* result);
    void Matrix4x4MultiplyAVXImpl(const f64* lhs, const f64* rhs, f64* result);
#endif
}

template<typename T>
static se::math::Matrix4x4Impl<T> CreateRandomMatrix(std::mt19937& gen)
{
    std::uniform_real_distribution<T> dist(static_cast<T>(-100.0), static_cast<T>(100.0));
    return se::math::Matrix4x4Impl<T>(
        dist(gen), dist(gen), dist(gen), dist(gen),
        dist(gen), dist(gen), dist(gen), dist(gen),
        dist(gen), dist(gen), dist(gen), dist(gen),
        dist(gen), dist(gen), dist(gen), dist(gen)
    );
}

// --- Float Benchmarks ---

// Benchmark for generic float matrix multiplication
static void BM_Matrix4x4Multiply_Generic_Float(benchmark::State& state)
{
    std::mt19937 gen(1234); // Fixed seed for reproducibility
    auto m1 = CreateRandomMatrix<f32>(gen);
    auto m2 = CreateRandomMatrix<f32>(gen);
    se::math::Matrix4x4Impl<f32> result;

    for ([[maybe_unused]] auto _ : state)
    {
        se::math::simd::detail::Matrix4x4MultiplyGeneric(m1.data.Data(), m2.data.Data(), result.data.Data());
        benchmark::DoNotOptimize(result); // Prevent the compiler from optimizing away the calculation.
    }
}
BENCHMARK(BM_Matrix4x4Multiply_Generic_Float);

#if SE_ARCH_X86_FAMILY
// Benchmark for SSE float matrix multiplication
static void BM_Matrix4x4Multiply_SSE_Float(benchmark::State& state)
{
    if (!se::CpuFeature::HasSSE4_1())
    {
        state.SkipWithError("SSE4.1 not supported");
        return;
    }
    std::mt19937 gen(1234);
    auto m1 = CreateRandomMatrix<f32>(gen);
    auto m2 = CreateRandomMatrix<f32>(gen);
    se::math::Matrix4x4Impl<f32> result;

    for ([[maybe_unused]] auto _ : state)
    {
        se::math::simd::detail::Matrix4x4MultiplySSEImpl(m1.data.Data(), m2.data.Data(), result.data.Data());
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Matrix4x4Multiply_SSE_Float);

// Benchmark for FMA float matrix multiplication
static void BM_Matrix4x4Multiply_FMA_Float(benchmark::State& state)
{
    if (!se::CpuFeature::HasFMA3())
    {
        state.SkipWithError("FMA3 not supported");
        return;
    }
    std::mt19937 gen(1234);
    auto m1 = CreateRandomMatrix<f32>(gen);
    auto m2 = CreateRandomMatrix<f32>(gen);
    se::math::Matrix4x4Impl<f32> result;

    for ([[maybe_unused]] auto _ : state)
    {
        se::math::simd::detail::Matrix4x4MultiplyFMAImpl(m1.data.Data(), m2.data.Data(), result.data.Data());
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Matrix4x4Multiply_FMA_Float);
#endif

// Benchmark for SIMD dispatch (operator*) float matrix multiplication
static void BM_Matrix4x4Multiply_Dispatch_Float(benchmark::State& state)
{
    std::mt19937 gen(1234);
    auto m1 = CreateRandomMatrix<f32>(gen);
    auto m2 = CreateRandomMatrix<f32>(gen);

    for ([[maybe_unused]] auto _ : state)
    {
        auto result = m1 * m2;
        benchmark::DoNotOptimize(result); // Prevent the compiler from optimizing away the calculation.
    }
}
BENCHMARK(BM_Matrix4x4Multiply_Dispatch_Float);


// --- Double Benchmarks ---

// Benchmark for generic f64 matrix multiplication
static void BM_Matrix4x4Multiply_Generic_Double(benchmark::State& state)
{
    std::mt19937 gen(1234);
    auto m1 = CreateRandomMatrix<f64>(gen);
    auto m2 = CreateRandomMatrix<f64>(gen);
    se::math::Matrix4x4Impl<f64> result;

    for ([[maybe_unused]] auto _ : state)
    {
        se::math::simd::detail::Matrix4x4MultiplyGeneric(m1.data.Data(), m2.data.Data(), result.data.Data());
        benchmark::DoNotOptimize(result); // Prevent the compiler from optimizing away the calculation.
    }
}
BENCHMARK(BM_Matrix4x4Multiply_Generic_Double);

#if SE_ARCH_X86_FAMILY
// Benchmark for AVX f64 matrix multiplication
static void BM_Matrix4x4Multiply_AVX_Double(benchmark::State& state)
{
    if (!se::CpuFeature::HasAVX() || !se::CpuFeature::HasFMA3())
    {
        state.SkipWithError("AVX or FMA3 not supported");
        return;
    }
    std::mt19937 gen(1234);
    auto m1 = CreateRandomMatrix<f64>(gen);
    auto m2 = CreateRandomMatrix<f64>(gen);
    se::math::Matrix4x4Impl<f64> result;

    for ([[maybe_unused]] auto _ : state)
    {
        se::math::simd::detail::Matrix4x4MultiplyAVXImpl(m1.data.Data(), m2.data.Data(), result.data.Data());
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Matrix4x4Multiply_AVX_Double);
#endif

// Benchmark for SIMD dispatch (operator*) f64 matrix multiplication
static void BM_Matrix4x4Multiply_Dispatch_Double(benchmark::State& state)
{
    std::mt19937 gen(1234);
    auto m1 = CreateRandomMatrix<f64>(gen);
    auto m2 = CreateRandomMatrix<f64>(gen);

    for ([[maybe_unused]] auto _ : state)
    {
        auto result = m1 * m2;
        benchmark::DoNotOptimize(result); // Prevent the compiler from optimizing away the calculation.
    }
}
BENCHMARK(BM_Matrix4x4Multiply_Dispatch_Double);
