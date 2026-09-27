#include "benchmark/benchmark.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"

#include <cstring>

// 쿡된 메시 크기의 정점 배열(1M개, 48MB)을 Packed로 쓰고 읽는 비용을 같은 바이트의 memcpy와 비교합니다.
// 버퍼와 대상 배열은 반복마다 재사용하므로 할당 비용은 빠지고 복사 경로만 비교됩니다.

namespace se
{
namespace
{
constexpr usize VERTEX_COUNT = 1'000'000;

/** 벤치마크용 정점 VERTEX_COUNT개를 만듭니다. */
[[nodiscard]] Array<StaticVertex> MakeBenchmarkVertices()
{
    Array<StaticVertex> vertices;
    vertices.Reserve(VERTEX_COUNT);
    for (usize i = 0; i < VERTEX_COUNT; ++i)
    {
        const f32 seed = static_cast<f32>(i);
        vertices.Push({
            .position = { seed, seed + 1.0f, seed + 2.0f },
            .normal = { 0.0f, 1.0f, 0.0f },
            .tex_coord = { seed * 0.5f, 0.25f },
            .tangent = { 1.0f, 0.0f, 0.0f, 1.0f },
        });
    }
    return vertices;
}

/** 반복마다 정점 배열 하나의 바이트를 처리한 것으로 기록합니다. */
void SetVertexBytesProcessed(benchmark::State& state)
{
    state.SetBytesProcessed(static_cast<i64>(state.iterations()) * static_cast<i64>(VERTEX_COUNT * sizeof(StaticVertex)));
}
} // namespace

/** 기준선: 같은 바이트를 memcpy로 옮깁니다. */
static void BM_VertexBytes_Memcpy(benchmark::State& state)
{
    const Array<StaticVertex> vertices = MakeBenchmarkVertices();
    Array<StaticVertex> copied;
    copied.ResizeUninitialized(VERTEX_COUNT);
    StaticVertex* destination = copied.Data();
    benchmark::DoNotOptimize(destination);

    for ([[maybe_unused]] auto _ : state)
    {
        std::memcpy(destination, vertices.Data(), VERTEX_COUNT * sizeof(StaticVertex));
        benchmark::ClobberMemory();
    }
    SetVertexBytesProcessed(state);
}
BENCHMARK(BM_VertexBytes_Memcpy)->Unit(benchmark::kMillisecond);

/** serde::Serialize로 정점 배열을 Packed에 씁니다. 원소 바이트를 RawElements로 한 번에 씁니다. */
static void BM_VertexArray_PackedSerialize(benchmark::State& state)
{
    const Array<StaticVertex> vertices = MakeBenchmarkVertices();
    Array<u8> buffer;

    for ([[maybe_unused]] auto _ : state)
    {
        buffer.Clear();
        PackedWriter writer(buffer);
        if (serde::Serialize(writer, vertices).HasError())
        {
            state.SkipWithError("Serialize failed.");
            break;
        }
        benchmark::ClobberMemory();
    }
    SetVertexBytesProcessed(state);
}
BENCHMARK(BM_VertexArray_PackedSerialize)->Unit(benchmark::kMillisecond);

/** serde::Deserialize로 Packed에서 정점 배열을 읽습니다. 원소 바이트를 RawElements로 한 번에 읽습니다. */
static void BM_VertexArray_PackedDeserialize(benchmark::State& state)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    if (serde::Serialize(writer, MakeBenchmarkVertices()).HasError())
    {
        state.SkipWithError("Serialize failed.");
        return;
    }

    Array<StaticVertex> vertices;
    for ([[maybe_unused]] auto _ : state)
    {
        PackedReader reader(buffer);
        if (serde::Deserialize(reader, vertices).HasError())
        {
            state.SkipWithError("Deserialize failed.");
            break;
        }
        benchmark::ClobberMemory();
    }
    SetVertexBytesProcessed(state);
}
BENCHMARK(BM_VertexArray_PackedDeserialize)->Unit(benchmark::kMillisecond);

/** 비교용: 같은 배열을 원소마다 serde::Serialize로 씁니다. 배열 경로를 거치지 않아 정점마다 필드별로 씁니다. */
static void BM_VertexArray_PackedSerializeElementByElement(benchmark::State& state)
{
    const Array<StaticVertex> vertices = MakeBenchmarkVertices();
    Array<u8> buffer;

    for ([[maybe_unused]] auto _ : state)
    {
        buffer.Clear();
        PackedWriter writer(buffer);
        writer.BeginSeq(vertices.Len(), ESeqOrder::Ordered);
        for (const StaticVertex& vertex : vertices)
        {
            if (serde::Serialize(writer, vertex).HasError())
            {
                state.SkipWithError("Serialize failed.");
                break;
            }
        }
        writer.EndSeq();
        benchmark::ClobberMemory();
    }
    SetVertexBytesProcessed(state);
}
BENCHMARK(BM_VertexArray_PackedSerializeElementByElement)->Unit(benchmark::kMillisecond);
} // namespace se
