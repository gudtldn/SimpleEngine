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

class SE_ANNOTATION(=meta::Internal) ScopeTestAsset : public AssetBase
{
    SE_CLASS(ScopeTestAsset, AssetBase)

public:
    ScopeTestAsset() = default;
};

SE_BEGIN_REFLECT(ScopeTestAsset, meta::Internal)
SE_END_REFLECT(ScopeTestAsset)


namespace
{
AssetId NewId() { return AssetId(Guid::NewGuid()); }

void DestroyAsset(void* p) { delete static_cast<ScopeTestAsset*>(p); }

/**
 * 슬롯에 에셋을 시뮬레이션 로드합니다.
 * @return 생성된 HandleData (ref_count == 0)
 */
HandleData SimulateLoad(
    AssetPool& pool, EScopeLayer scope,
    uint64 size_bytes = 256, uint64 frame = 0)
{
    HandleData hd = pool.FindOrCreate(NewId(), TypeId::Get<ScopeTestAsset>(), AssetPath("test/scope_asset"));

    HandleTable& table = pool.GetTable();
    SlotEntry& slot = table.GetSlot(hd.index);

    slot.destructor = &DestroyAsset;
    (void)slot.ExchangeAsset(new ScopeTestAsset());
    slot.SetState(ELoadingState::Loaded);
    slot.asset_size_bytes = size_bytes;
    slot.last_access_frame = frame;
    slot.scope = scope;
    table.TrackMemoryUsage(size_bytes);

    return hd;
}
}


// =============================================================================
// Scope Layer Tests (M6)
// =============================================================================

class ScopeLayerTest : public ::testing::Test
{
protected:
    AssetPool pool;
};

TEST_F(ScopeLayerTest, UnloadScope_EvictsMatchingScope)
{
    SimulateLoad(pool, EScopeLayer::Scene);
    SimulateLoad(pool, EScopeLayer::Scene);
    SimulateLoad(pool, EScopeLayer::Session);

    EXPECT_EQ(pool.GetCount(), 3u);

    uint32 evicted = pool.UnloadScope(EScopeLayer::Scene);
    EXPECT_EQ(evicted, 2u);
    EXPECT_EQ(pool.GetCount(), 1u);
}

TEST_F(ScopeLayerTest, UnloadScope_DoesNotAffectOtherScopes)
{
    HandleData global_hd = SimulateLoad(pool, EScopeLayer::Global);
    HandleData session_hd = SimulateLoad(pool, EScopeLayer::Session);
    SimulateLoad(pool, EScopeLayer::Scene);

    pool.UnloadScope(EScopeLayer::Scene);

    EXPECT_TRUE(pool.GetTable().IsHandleValid(global_hd));
    EXPECT_TRUE(pool.GetTable().IsHandleValid(session_hd));
}

TEST_F(ScopeLayerTest, UnloadScope_SkipsActiveHandles)
{
    HandleData hd = SimulateLoad(pool, EScopeLayer::Scene);

    // AssetHandle 생성 -> ref_count = 1
    AssetHandle<ScopeTestAsset> handle(hd, &pool.GetTable());

    uint32 evicted = pool.UnloadScope(EScopeLayer::Scene);
    EXPECT_EQ(evicted, 0u);
    EXPECT_EQ(pool.GetCount(), 1u);
}

TEST_F(ScopeLayerTest, UnloadScope_DecrementsMemoryUsage)
{
    SimulateLoad(pool, EScopeLayer::Scene, 1024);
    SimulateLoad(pool, EScopeLayer::Scene, 2048);
    EXPECT_EQ(pool.GetTotalMemoryUsage(), 3072u);

    pool.UnloadScope(EScopeLayer::Scene);

    EXPECT_EQ(pool.GetTotalMemoryUsage(), 0u);
}

TEST_F(ScopeLayerTest, UnloadScope_GlobalGuarded)
{
    SimulateLoad(pool, EScopeLayer::Global, 1024);

    // Global은 다른 scope 해제에 영향받지 않음
    pool.UnloadScope(EScopeLayer::Scene);
    pool.UnloadScope(EScopeLayer::Session);
    pool.UnloadScope(EScopeLayer::Transient);

    EXPECT_EQ(pool.GetCount(), 1u);
    EXPECT_EQ(pool.GetTotalMemoryUsage(), 1024u);
}

TEST_F(ScopeLayerTest, DefaultScopeIsScene)
{
    HandleData hd = pool.FindOrCreate(NewId(), TypeId::Get<ScopeTestAsset>(), AssetPath("test/default"));
    SlotEntry& slot = pool.GetTable().GetSlot(hd.index);

    // SlotEntry의 기본 scope는 Scene
    EXPECT_EQ(slot.scope, EScopeLayer::Scene);
}

TEST_F(ScopeLayerTest, UnloadScope_TransientOnly)
{
    HandleData scene_hd = SimulateLoad(pool, EScopeLayer::Scene);
    HandleData transient_hd = SimulateLoad(pool, EScopeLayer::Transient);

    uint32 evicted = pool.UnloadScope(EScopeLayer::Transient);
    EXPECT_EQ(evicted, 1u);

    EXPECT_TRUE(pool.GetTable().IsHandleValid(scene_hd));
    EXPECT_FALSE(pool.GetTable().IsHandleValid(transient_hd));
}

TEST_F(ScopeLayerTest, UnloadScope_SessionBulkRelease)
{
    SimulateLoad(pool, EScopeLayer::Session, 512);
    SimulateLoad(pool, EScopeLayer::Session, 512);
    SimulateLoad(pool, EScopeLayer::Session, 512);
    SimulateLoad(pool, EScopeLayer::Scene, 256);

    uint32 evicted = pool.UnloadScope(EScopeLayer::Session);
    EXPECT_EQ(evicted, 3u);
    EXPECT_EQ(pool.GetCount(), 1u);
    EXPECT_EQ(pool.GetTotalMemoryUsage(), 256u);
}
