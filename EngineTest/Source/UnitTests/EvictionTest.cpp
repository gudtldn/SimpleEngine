#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetPool.h"
#include "SimpleEngine/Asset/SlotEntry.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Core/Types/Guid.h"

using namespace se;
using namespace se::asset;


// --- Mock Asset ---

class SE_ANNOTATION(=meta::Internal) EvictionTestAsset : public AssetBase
{
    SE_CLASS(EvictionTestAsset, AssetBase)

public:
    EvictionTestAsset() = default;
};

SE_BEGIN_REFLECT(EvictionTestAsset, meta::Internal)
SE_END_REFLECT(EvictionTestAsset)


namespace
{
AssetId NewId() { return AssetId(Guid::NewGuid()); }

void DestroyAsset(void* p) { delete static_cast<EvictionTestAsset*>(p); }

/**
 * 슬롯에 에셋을 시뮬레이션 로드합니다.
 * @return 생성된 HandleData (ref_count == 0)
 */
HandleData SimulateLoad(
    AssetPool& pool, uint64 size_bytes, uint64 frame,
    EScopeLayer scope = EScopeLayer::Scene)
{
    HandleData hd = pool.FindOrCreate(NewId(), TypeId::Get<EvictionTestAsset>(), AssetPath("test/eviction_asset"));

    HandleTable& table = pool.GetTable();
    SlotEntry& slot = table.GetSlot(hd.index);

    slot.destructor = &DestroyAsset;
    (void)slot.ExchangeAsset(new EvictionTestAsset());
    slot.SetState(ELoadingState::Loaded);
    slot.asset_size_bytes = size_bytes;
    slot.last_access_frame = frame;
    slot.scope = scope;
    table.TrackMemoryUsage(size_bytes);

    return hd;
}
}


// =============================================================================
// Eviction Tests (M5)
// =============================================================================

class EvictionTest : public ::testing::Test
{
protected:
    AssetPool pool;
};

// === Memory Tracking ===

TEST_F(EvictionTest, TrackMemoryUsage_InitialZero)
{
    EXPECT_EQ(pool.GetTotalMemoryUsage(), 0u);
}

TEST_F(EvictionTest, TrackMemoryUsage_AccumulatesOnLoad)
{
    SimulateLoad(pool, 1024, 0);
    EXPECT_EQ(pool.GetTotalMemoryUsage(), 1024u);

    SimulateLoad(pool, 2048, 0);
    EXPECT_EQ(pool.GetTotalMemoryUsage(), 3072u);
}

TEST_F(EvictionTest, TrackMemoryUsage_DecrementsOnEvict)
{
    SimulateLoad(pool, 1024, 0);
    SimulateLoad(pool, 2048, 0);

    pool.CollectGarbage(); // ref_count == 0이므로 모두 해제
    EXPECT_EQ(pool.GetTotalMemoryUsage(), 0u);
    EXPECT_EQ(pool.GetCount(), 0u);
}

// === PendingDestroy Lifecycle ===

TEST_F(EvictionTest, DeferDestroy_ProcessesAfterReleaseFrame)
{
    auto* asset = new EvictionTestAsset();

    // Frame 10에서 추가 -> release_frame = 11
    pool.DeferDestroy(AssetPayload{ asset, &DestroyAsset }, 10);

    // Frame 10: 10 < 11 -> 아직 해제 안 됨
    pool.ProcessPendingDestroy(10);
    // 여전히 pending에 남아있음 (직접 검증 불가하므로, 다음 호출로 간접 확인)

    // Frame 11: 11 <= 11 -> 해제됨 (no crash = success)
    pool.ProcessPendingDestroy(11);
}

TEST_F(EvictionTest, DeferDestroy_NullPayloadIgnored)
{
    pool.DeferDestroy(AssetPayload{ nullptr, nullptr }, 0);
    pool.ProcessPendingDestroy(100); // No crash
}

// === EvictIfOverBudget ===

TEST_F(EvictionTest, EvictIfOverBudget_NothingWhenUnderBudget)
{
    pool.SetMemoryBudget(4096);
    SimulateLoad(pool, 1024, 0);

    EXPECT_EQ(pool.EvictIfOverBudget(100), 0u);
    EXPECT_EQ(pool.GetCount(), 1u);
}

TEST_F(EvictionTest, EvictIfOverBudget_EvictsWhenOverBudget)
{
    pool.SetMemoryBudget(1000);
    pool.SetGraceFrames(0);

    SimulateLoad(pool, 512, 0);
    SimulateLoad(pool, 512, 0);
    SimulateLoad(pool, 512, 0);

    // Total: 1536 > 1000
    uint32 evicted = pool.EvictIfOverBudget(100);
    EXPECT_GT(evicted, 0u);
    EXPECT_LE(pool.GetTotalMemoryUsage(), 1000u);
}

TEST_F(EvictionTest, EvictIfOverBudget_RespectsGracePeriod)
{
    pool.SetMemoryBudget(500);
    pool.SetGraceFrames(10);

    SimulateLoad(pool, 1024, 5); // last_access_frame = 5

    // Frame 10: 5 + 10 = 15 > 10 -> grace period 보호
    EXPECT_EQ(pool.EvictIfOverBudget(10), 0u);
    EXPECT_EQ(pool.GetCount(), 1u);

    // Frame 16: 5 + 10 = 15 <= 16 -> grace period 만료, 해제 가능
    EXPECT_EQ(pool.EvictIfOverBudget(16), 1u);
    EXPECT_EQ(pool.GetCount(), 0u);
}

TEST_F(EvictionTest, EvictIfOverBudget_GlobalNeverEvicted)
{
    pool.SetMemoryBudget(500);
    pool.SetGraceFrames(0);

    SimulateLoad(pool, 1024, 0, EScopeLayer::Global);

    // Global scope는 EvictIfOverBudget에서 절대 해제하지 않음
    EXPECT_EQ(pool.EvictIfOverBudget(100), 0u);
    EXPECT_EQ(pool.GetCount(), 1u);
}

TEST_F(EvictionTest, EvictIfOverBudget_TransientEvictedFirst)
{
    pool.SetMemoryBudget(1000);
    pool.SetGraceFrames(0);

    HandleData scene_hd = SimulateLoad(pool, 512, 0, EScopeLayer::Scene);
    HandleData transient_hd = SimulateLoad(pool, 512, 0, EScopeLayer::Transient);

    // Total: 1024 > 1000 -> Transient이 먼저 해제되어야 함
    pool.EvictIfOverBudget(100);

    EXPECT_FALSE(pool.GetTable().IsHandleValid(transient_hd));
    // 512 <= 1000이므로 Scene은 유지
    EXPECT_TRUE(pool.GetTable().IsHandleValid(scene_hd));
}

TEST_F(EvictionTest, EvictIfOverBudget_RespectsMaxEvictions)
{
    pool.SetMemoryBudget(0); // 모든 것을 해제하려 함
    pool.SetGraceFrames(0);
    pool.SetMaxEvictionsPerFrame(2);

    SimulateLoad(pool, 100, 0);
    SimulateLoad(pool, 100, 0);
    SimulateLoad(pool, 100, 0);
    SimulateLoad(pool, 100, 0);

    uint32 evicted = pool.EvictIfOverBudget(100);
    EXPECT_LE(evicted, 2u);
    // 나머지는 다음 프레임에 해제됨
    EXPECT_EQ(pool.GetCount(), 2u);
}

TEST_F(EvictionTest, EvictIfOverBudget_ActiveHandlesProtected)
{
    pool.SetMemoryBudget(500);
    pool.SetGraceFrames(0);

    HandleData hd = SimulateLoad(pool, 1024, 0);

    // AssetHandle 생성 -> ref_count = 1
    AssetHandle<EvictionTestAsset> handle(hd, &pool.GetTable());

    // Handle이 있으므로 해제 불가
    EXPECT_EQ(pool.EvictIfOverBudget(100), 0u);
    EXPECT_EQ(pool.GetCount(), 1u);
}
