#include "benchmark/benchmark.h"

#include "SimpleEngine/Asset/AssetPayload.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"
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

/** DDC가 payload를 쓰고 읽을 때마다 계산하는 StaticMesh의 스키마 해시입니다. */
static void BM_StaticMesh_SchemaHash(benchmark::State& state)
{
    const SerializePlan& plan = SerializePlanOf<StaticMesh>();
    for ([[maybe_unused]] auto _ : state)
    {
        u64 hash = plan.SchemaHash();
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BM_StaticMesh_SchemaHash)->Unit(benchmark::kMicrosecond);

/** 아래 DDC payload 벤치마크의 기준선: 반복마다 새 버퍼를 할당해 같은 바이트를 memcpy로 옮깁니다. */
static void BM_VertexBytes_MemcpyToNewBuffer(benchmark::State& state)
{
    const Array<StaticVertex> vertices = MakeBenchmarkVertices();

    for ([[maybe_unused]] auto _ : state)
    {
        Array<StaticVertex> copied;
        copied.ResizeUninitialized(VERTEX_COUNT);
        std::memcpy(copied.Data(), vertices.Data(), VERTEX_COUNT * sizeof(StaticVertex));
        benchmark::DoNotOptimize(copied);
    }
    SetVertexBytesProcessed(state);
}
BENCHMARK(BM_VertexBytes_MemcpyToNewBuffer)->Unit(benchmark::kMillisecond);

/** 정점 1M개를 담은 StaticMesh를 DDC payload로 씁니다. 버퍼 할당, 스키마 해시, 체크섬이 포함됩니다. */
static void BM_StaticMesh_SerializeAssetPayload(benchmark::State& state)
{
    StaticMesh mesh;
    mesh.vertices = MakeBenchmarkVertices();

    for ([[maybe_unused]] auto _ : state)
    {
        Array<u8> payload = AssetSubsystem::SerializeAssetPayload(mesh);
        benchmark::DoNotOptimize(payload);
    }
    SetVertexBytesProcessed(state);
}
BENCHMARK(BM_StaticMesh_SerializeAssetPayload)->Unit(benchmark::kMillisecond);

/** 정점 1M개를 담은 StaticMesh DDC payload를 읽습니다. 헤더 검증, 체크섬, 객체 생성이 포함되고 소멸은 빠집니다. */
static void BM_StaticMesh_DeserializeAssetPayload(benchmark::State& state)
{
    StaticMesh mesh;
    mesh.vertices = MakeBenchmarkVertices();
    const Array<u8> payload = AssetSubsystem::SerializeAssetPayload(mesh);

    for ([[maybe_unused]] auto _ : state)
    {
        const AssetPayload loaded = AssetSubsystem::DeserializeAssetPayload(TypeId_v1::Of<StaticMesh>(), payload);
        if (!loaded.IsValid())
        {
            state.SkipWithError("DeserializeAssetPayload failed.");
            break;
        }

        state.PauseTiming();
        loaded.destructor(loaded.ptr);
        state.ResumeTiming();
    }
    SetVertexBytesProcessed(state);
}
BENCHMARK(BM_StaticMesh_DeserializeAssetPayload)->Unit(benchmark::kMillisecond);
} // namespace se
