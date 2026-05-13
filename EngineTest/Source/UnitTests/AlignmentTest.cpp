#include "gtest/gtest.h"

#include <cstdint>

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Concurrency/JobHandle.h"
#include "SimpleEngine/Core/Concurrency/JobPayload.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Concurrency/MpscTaskLinkedQueue.h"
#include "SimpleEngine/Core/Concurrency/WorkStealingDeque.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/ECS/SparseSet.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"

using namespace se;
using namespace math;


// ============================================================================
// SparseSet Alignment Tests
// ============================================================================

class SparseSetAlignmentTest : public ::testing::Test {};

struct alignas(16) SimdComponent
{
    f32 data[4]{};
};

struct alignas(32) Avx2Component
{
    f32 data[8]{};
};

TEST_F(SparseSetAlignmentTest, Alignas16ComponentMaintainsAlignment)
{
    EntityManager mgr;
    SparseSet<SimdComponent> sparse;

    for (u32 i = 0; i < 64; ++i)
    {
        SimdComponent comp{};
        comp.data[0] = static_cast<f32>(i);
        sparse.Add(mgr.Create(), comp);
    }

    const auto& components = sparse.GetComponents();
    for (usize i = 0; i < components.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&components[i]);
        EXPECT_EQ(addr % 16, 0u) << "Component at index " << i << " is not 16-byte aligned";
    }
}

TEST_F(SparseSetAlignmentTest, Alignas32ComponentMaintainsAlignment)
{
    EntityManager mgr;
    SparseSet<Avx2Component> sparse;

    for (u32 i = 0; i < 32; ++i)
    {
        Avx2Component comp{};
        comp.data[0] = static_cast<f32>(i);
        sparse.Add(mgr.Create(), comp);
    }

    const auto& components = sparse.GetComponents();
    for (usize i = 0; i < components.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&components[i]);
        EXPECT_EQ(addr % 32, 0u) << "Component at index " << i << " is not 32-byte aligned";
    }
}

TEST_F(SparseSetAlignmentTest, AlignmentPreservedAfterSwapRemove)
{
    EntityManager mgr;
    SparseSet<SimdComponent> sparse;

    Array<Entity> entities;
    for (u32 i = 0; i < 16; ++i)
    {
        Entity e = mgr.Create();
        SimdComponent comp{};
        comp.data[0] = static_cast<f32>(i);
        sparse.Add(e, comp);
        entities.Push(e);
    }

    // 중간 엔티티들을 제거하여 swap-remove 발동
    sparse.Remove(entities[3]);
    sparse.Remove(entities[7]);
    sparse.Remove(entities[11]);

    const auto& components = sparse.GetComponents();
    for (usize i = 0; i < components.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&components[i]);
        EXPECT_EQ(addr % 16, 0u) << "Component at index " << i << " lost alignment after swap-remove";
    }
}


// ============================================================================
// OsMemory Alignment Tests
// ============================================================================

class OsMemoryAlignmentTest : public ::testing::Test {};

TEST_F(OsMemoryAlignmentTest, DefaultAlignment)
{
    void* ptr = OsMemory::Allocate(128);
    ASSERT_NE(ptr, nullptr);

    auto addr = reinterpret_cast<usize>(ptr);
    EXPECT_EQ(addr % alignof(std::max_align_t), 0u);

    OsMemory::Free(ptr);
}

TEST_F(OsMemoryAlignmentTest, SimdAlignment16)
{
    void* ptr = OsMemory::Allocate(256, 16);
    ASSERT_NE(ptr, nullptr);

    auto addr = reinterpret_cast<usize>(ptr);
    EXPECT_EQ(addr % 16, 0u);

    OsMemory::Free(ptr);
}

TEST_F(OsMemoryAlignmentTest, AvxAlignment32)
{
    void* ptr = OsMemory::Allocate(256, 32);
    ASSERT_NE(ptr, nullptr);

    auto addr = reinterpret_cast<usize>(ptr);
    EXPECT_EQ(addr % 32, 0u);

    OsMemory::Free(ptr);
}

TEST_F(OsMemoryAlignmentTest, CacheLineAlignment)
{
    void* ptr = OsMemory::Allocate(512, SE_CACHE_LINE);
    ASSERT_NE(ptr, nullptr);

    auto addr = reinterpret_cast<usize>(ptr);
    EXPECT_EQ(addr % SE_CACHE_LINE, 0u);

    OsMemory::Free(ptr);
}

TEST_F(OsMemoryAlignmentTest, OverAligned64)
{
    void* ptr = OsMemory::Allocate(1024, 64);
    ASSERT_NE(ptr, nullptr);

    auto addr = reinterpret_cast<usize>(ptr);
    EXPECT_EQ(addr % 64, 0u);

    OsMemory::Free(ptr);
}

TEST_F(OsMemoryAlignmentTest, TypedAllocateRespectsAlignof)
{
    auto* ptr = OsMemory::Allocate<SimdComponent>(4);
    ASSERT_NE(ptr, nullptr);

    for (usize i = 0; i < 4; ++i)
    {
        auto addr = reinterpret_cast<usize>(&ptr[i]);
        EXPECT_EQ(addr % alignof(SimdComponent), 0u) << "Element " << i << " not aligned";
    }

    OsMemory::Free(ptr);
}

TEST_F(OsMemoryAlignmentTest, ReallocPreservesAlignment)
{
    void* ptr = OsMemory::Allocate(64, 32);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<usize>(ptr) % 32, 0u);

    void* new_ptr = OsMemory::Realloc(ptr, 256, 32);
    ASSERT_NE(new_ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<usize>(new_ptr) % 32, 0u);

    OsMemory::Free(new_ptr);
}


// ============================================================================
// JobPayload SBO Alignment Tests
// ============================================================================

class JobPayloadAlignmentTest : public ::testing::Test {};

TEST_F(JobPayloadAlignmentTest, SmallLambdaUsesInlineStorage)
{
    int captured = 42;
    auto* payload = JobPayload::Create([captured]() mutable { captured++; });

    EXPECT_EQ(payload->storage_type, JobPayload::EStorageType::Inline);
    auto storage_addr = reinterpret_cast<usize>(payload->GetStorage());
    EXPECT_EQ(storage_addr % JobPayload::SBO_ALIGNMENT, 0u);

    delete payload;
}

TEST_F(JobPayloadAlignmentTest, OversizedLambdaUsesPooledStorage)
{
    // SBO_CAPACITY(48바이트)를 초과하지만 16바이트 정렬인 캡처
    // NOLINTBEGIN(*-avoid-c-arrays)
    char big_capture[64]{};
    // NOLINTEND(*-avoid-c-arrays)
    auto* payload = JobPayload::Create([big_capture]() { (void)big_capture; });

    EXPECT_EQ(payload->storage_type, JobPayload::EStorageType::Pooled);
    auto storage_addr = reinterpret_cast<usize>(payload->GetStorage());
    EXPECT_EQ(storage_addr % JobPayload::SBO_ALIGNMENT, 0u);

    delete payload;
}

TEST_F(JobPayloadAlignmentTest, OverAlignedLambdaUsesHeapStorage)
{
    struct alignas(32) OverAligned
    {
        int value = 0;
    };

    OverAligned oa{};
    auto* payload = JobPayload::Create([oa]() { (void)oa; });

    EXPECT_EQ(payload->storage_type, JobPayload::EStorageType::Heap);
    auto storage_addr = reinterpret_cast<usize>(payload->GetStorage());
    EXPECT_EQ(storage_addr % 32, 0u);

    delete payload;
}

TEST_F(JobPayloadAlignmentTest, InlineBoundaryExactFit)
{
    // 정확히 SBO_CAPACITY 이하 + 16바이트 이하 정렬 → Inline
    struct alignas(16) ExactFit
    {
        // NOLINTNEXTLINE(*-avoid-c-arrays)
        u8 data[JobPayload::SBO_CAPACITY - sizeof(void*)]{};
        void* ptr = nullptr;
    };
    static_assert(sizeof(ExactFit) <= JobPayload::SBO_CAPACITY);
    static_assert(alignof(ExactFit) <= JobPayload::SBO_ALIGNMENT);

    ExactFit ef{};
    auto* payload = JobPayload::Create([ef]() { (void)ef; });

    EXPECT_EQ(payload->storage_type, JobPayload::EStorageType::Inline);

    delete payload;
}


// ============================================================================
// SIMD Type Array Storage Alignment Tests
// ============================================================================

class SimdArrayAlignmentTest : public ::testing::Test {};

TEST_F(SimdArrayAlignmentTest, Matrix4x4InArray)
{
    Array<Matrix4x4f> matrices;
    for (int i = 0; i < 16; ++i)
    {
        matrices.Push(Matrix4x4f::Identity());
    }

    constexpr usize REQUIRED_ALIGNMENT = alignof(Matrix4x4f);
    for (usize i = 0; i < matrices.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&matrices[i]);
        EXPECT_EQ(addr % REQUIRED_ALIGNMENT, 0u) << "Matrix at index " << i << " misaligned";
    }
}

TEST_F(SimdArrayAlignmentTest, QuaternionInArray)
{
    Array<Quaternionf> quats;
    for (int i = 0; i < 32; ++i)
    {
        quats.Push(Quaternionf::Identity());
    }

    for (usize i = 0; i < quats.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&quats[i]);
        EXPECT_EQ(addr % alignof(Quaternionf), 0u) << "Quaternion at index " << i << " misaligned";
    }
}

TEST_F(SimdArrayAlignmentTest, Vector4InArray)
{
    Array<Vector4f> vectors;
    for (int i = 0; i < 32; ++i)
    {
        vectors.Push(Vector4f(1.0f, 2.0f, 3.0f, 4.0f));
    }

    for (usize i = 0; i < vectors.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&vectors[i]);
        EXPECT_EQ(addr % alignof(Vector4f), 0u) << "Vector4 at index " << i << " misaligned";
    }
}

TEST_F(SimdArrayAlignmentTest, StaticVertexInArray)
{
    Array<StaticVertex> vertices;
    for (int i = 0; i < 64; ++i)
    {
        StaticVertex v{};
        v.position = Vector3f(1.0f, 2.0f, 3.0f);
        vertices.Push(v);
    }

    for (usize i = 0; i < vertices.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&vertices[i]);
        EXPECT_EQ(addr % alignof(StaticVertex), 0u) << "StaticVertex at index " << i << " misaligned";
    }
}

TEST_F(SimdArrayAlignmentTest, SimdTypeInSparseSet)
{
    EntityManager mgr;
    SparseSet<Quaternionf> sparse;

    for (u32 i = 0; i < 32; ++i)
    {
        sparse.Add(mgr.Create(), Quaternionf::Identity());
    }

    const auto& components = sparse.GetComponents();
    for (usize i = 0; i < components.Len(); ++i)
    {
        auto addr = reinterpret_cast<usize>(&components[i]);
        EXPECT_EQ(addr % alignof(Quaternionf), 0u) << "Quaternion in SparseSet at index " << i << " misaligned";
    }
}


// ============================================================================
// Cache Line False Sharing Prevention Tests
// ============================================================================

class CacheLineFalseSharingTest : public ::testing::Test {};

TEST_F(CacheLineFalseSharingTest, JobCounterSizeSeparatesCacheLines)
{
    // JobCounter는 count와 waiters를 각각 별도 캐시 라인에 배치합니다.
    // 두 필드가 독립된 캐시 라인에 있으려면 sizeof >= 2 * SE_CACHE_LINE이어야 합니다.
    static_assert(sizeof(JobCounter) >= 2 * SE_CACHE_LINE);
    static_assert(alignof(JobCounter) >= SE_CACHE_LINE);
}

TEST_F(CacheLineFalseSharingTest, WorkStealingDequeSizeSeparatesCacheLines)
{
    // WorkStealingDeque는 top, bottom, buffer를 각각 별도 캐시 라인에 배치합니다.
    // 세 필드가 독립된 캐시 라인에 있으려면 sizeof >= 3 * SE_CACHE_LINE이어야 합니다.
    using Deque = WorkStealingDeque<int>;
    static_assert(sizeof(Deque) >= 3 * SE_CACHE_LINE);
    static_assert(alignof(Deque) >= SE_CACHE_LINE);
}
