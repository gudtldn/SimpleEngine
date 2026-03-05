#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetPool.h"
#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/AssetSlot.h"
#include "SimpleEngine/Asset/SlotEntry.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/VPath.h"

using namespace se;
using namespace se::asset;

namespace
{
// --- Helper Function ---
AssetId GenerateAssetId()
{
    return AssetId(Guid::NewGuid());
}
}

// --- Mock Assets for Testing ---

class SE_ANNOTATION(=meta::Internal) MockTexture : public AssetBase
{
    SE_CLASS(MockTexture, AssetBase)

public:
    MockTexture() = default;

    int width = 1024;
    int height = 1024;
};

SE_BEGIN_REFLECT(MockTexture, meta::Internal)
SE_END_REFLECT(MockTexture)

class SE_ANNOTATION(=meta::Internal) MockMesh : public AssetBase
{
    SE_CLASS(MockMesh, AssetBase)

public:
    MockMesh() = default;

    uint32 vertex_count = 100;
};

SE_BEGIN_REFLECT(MockMesh, meta::Internal)
SE_END_REFLECT(MockMesh)

// =============================================================================
// AssetPath Tests
// =============================================================================

class AssetPathTest : public ::testing::Test {};

TEST_F(AssetPathTest, Construction_FilePathOnly)
{
    AssetPath path("textures/diffuse.png");

    EXPECT_EQ(path.GetFilePath().ToString(), "textures/diffuse.png");
    EXPECT_TRUE(path.GetSubAssetName().IsEmpty());
    EXPECT_FALSE(path.HasSubAsset());
}

TEST_F(AssetPathTest, Construction_WithSubAsset)
{
    AssetPath path("models/character.fbx#MainMesh");

    EXPECT_EQ(path.GetFilePath().ToString(), "models/character.fbx");
    EXPECT_EQ(path.GetSubAssetName(), "MainMesh");
    EXPECT_TRUE(path.HasSubAsset());
}

TEST_F(AssetPathTest, Construction_Separate)
{
    AssetPath path(VPath("models/weapon.obj"), "Blade");

    EXPECT_EQ(path.GetFilePath().ToString(), "models/weapon.obj");
    EXPECT_EQ(path.GetSubAssetName(), "Blade");
    EXPECT_TRUE(path.HasSubAsset());
}

TEST_F(AssetPathTest, ToString)
{
    AssetPath path1("textures/normal.png");
    EXPECT_EQ(path1.ToString(), "textures/normal.png");

    AssetPath path2("models/car.fbx#Wheel");
    EXPECT_EQ(path2.ToString(), "models/car.fbx#Wheel");
}

TEST_F(AssetPathTest, Equality)
{
    AssetPath path1("models/test.fbx#Mesh");
    AssetPath path2("models/test.fbx#Mesh");
    AssetPath path3("models/test.fbx#Other");

    EXPECT_EQ(path1, path2);
    EXPECT_NE(path1, path3);
}

TEST_F(AssetPathTest, Hash)
{
    AssetPath path1("textures/test.png");
    AssetPath path2("textures/test.png");
    AssetPath path3("textures/other.png");

    std::hash<AssetPath> hasher;
    EXPECT_EQ(hasher(path1), hasher(path2));
    EXPECT_NE(hasher(path1), hasher(path3));
}

// =============================================================================
// AssetSlot Tests
// =============================================================================

class AssetSlotTest : public ::testing::Test {};

TEST_F(AssetSlotTest, InitialState)
{
    AssetId id = GenerateAssetId();
    TypeId type_id = TypeId::Get<MockTexture>();
    AssetPath path("textures/test.png");

    AssetSlot slot(id, type_id, path);

    EXPECT_EQ(slot.GetAssetId(), id);
    EXPECT_EQ(slot.GetAssetType(), type_id);
    EXPECT_EQ(slot.GetSourcePath().ToString(), path.ToString());
    EXPECT_EQ(slot.GetState(), ELoadingState::Unloaded);
    EXPECT_EQ(slot.GetRawAsset(), nullptr);
}

TEST_F(AssetSlotTest, ExchangeAsset)
{
    AssetId id = GenerateAssetId();
    AssetSlot slot(id, TypeId::Get<MockTexture>(), AssetPath("test.png"));

    auto asset = std::make_shared<MockTexture>();
    asset->width = 512;

    // Exchange asset
    auto old_asset = slot.ExchangeAsset(asset, ELoadingState::Loaded);

    EXPECT_EQ(old_asset, nullptr);  // No previous asset
    EXPECT_EQ(slot.GetState(), ELoadingState::Loaded);
    EXPECT_NE(slot.GetRawAsset(), nullptr);
    EXPECT_EQ(static_cast<MockTexture*>(slot.GetRawAsset())->width, 512);
}

TEST_F(AssetSlotTest, GetAsset_SharedPointer)
{
    AssetId id = GenerateAssetId();
    AssetSlot slot(id, TypeId::Get<MockMesh>(), AssetPath("mesh.obj"));

    auto asset = std::make_shared<MockMesh>();
    asset->vertex_count = 200;

    (void)slot.ExchangeAsset(asset, ELoadingState::Loaded);

    auto retrieved = slot.GetAsset();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(std::static_pointer_cast<MockMesh>(retrieved)->vertex_count, 200);
}

TEST_F(AssetSlotTest, StateTransitions)
{
    AssetId id = GenerateAssetId();
    AssetSlot slot(id, TypeId::Get<MockTexture>(), AssetPath("test.png"));

    EXPECT_EQ(slot.GetState(), ELoadingState::Unloaded);

    slot.SetState(ELoadingState::Loading);
    EXPECT_EQ(slot.GetState(), ELoadingState::Loading);

    auto asset = std::make_shared<MockTexture>();
    (void)slot.ExchangeAsset(asset, ELoadingState::Loaded);
    EXPECT_EQ(slot.GetState(), ELoadingState::Loaded);

    slot.SetState(ELoadingState::Failed);
    EXPECT_EQ(slot.GetState(), ELoadingState::Failed);
}

TEST_F(AssetSlotTest, Invalidate)
{
    AssetId id = GenerateAssetId();
    AssetSlot slot(id, TypeId::Get<MockTexture>(), AssetPath("test.png"));

    auto asset = std::make_shared<MockTexture>();
    (void)slot.ExchangeAsset(asset, ELoadingState::Loaded);

    ASSERT_NE(slot.GetRawAsset(), nullptr);

    auto old = slot.Invalidate();
    EXPECT_NE(old, nullptr);
    EXPECT_EQ(slot.GetRawAsset(), nullptr);
    EXPECT_EQ(slot.GetState(), ELoadingState::Unloaded);
}

TEST_F(AssetSlotTest, LockFreeRead)
{
    AssetId id = GenerateAssetId();
    AssetSlot slot(id, TypeId::Get<MockTexture>(), AssetPath("test.png"));

    auto asset = std::make_shared<MockTexture>();
    asset->width = 2048;
    (void)slot.ExchangeAsset(asset, ELoadingState::Loaded);

    // GetRawAsset should be lock-free (atomic read)
    for (int i = 0; i < 100; ++i)
    {
        AssetBase* raw = slot.GetRawAsset();
        ASSERT_NE(raw, nullptr);
        EXPECT_EQ(static_cast<MockTexture*>(raw)->width, 2048);
    }
}

// =============================================================================
// AssetPool Tests
// =============================================================================

class AssetCacheTest : public ::testing::Test
{
protected:
    AssetPool cache;
};

TEST_F(AssetCacheTest, FindOrCreate_NewSlot)
{
    AssetId id = GenerateAssetId();
    TypeId type = TypeId::Get<MockTexture>();
    AssetPath path("textures/new.png");

    HandleData hd = cache.FindOrCreate(id, type, path);

    ASSERT_TRUE(hd.IsValid());
    const SlotEntry& slot = cache.GetTable().GetSlot(hd.index);
    EXPECT_EQ(slot.asset_id, id);
    EXPECT_EQ(slot.asset_type, type);
    EXPECT_EQ(slot.source_path, path);
    EXPECT_EQ(cache.GetCount(), 1u);
}

TEST_F(AssetCacheTest, FindOrCreate_ExistingSlot)
{
    AssetId id = GenerateAssetId();
    TypeId type = TypeId::Get<MockTexture>();
    AssetPath path("textures/existing.png");

    HandleData hd1 = cache.FindOrCreate(id, type, path);
    HandleData hd2 = cache.FindOrCreate(id, type, path);

    EXPECT_EQ(hd1, hd2);  // Same handle
    EXPECT_EQ(cache.GetCount(), 1u);
}

TEST_F(AssetCacheTest, Find_NonExistent)
{
    AssetId id = GenerateAssetId();
    auto result = cache.Find(id);

    EXPECT_FALSE(result.HasValue());
}

TEST_F(AssetCacheTest, Find_Existing)
{
    AssetId id = GenerateAssetId();
    HandleData created = cache.FindOrCreate(id, TypeId::Get<MockMesh>(), AssetPath("mesh.obj"));

    auto found = cache.Find(id);
    ASSERT_TRUE(found.HasValue());
    EXPECT_EQ(found.Value(), created);
}

TEST_F(AssetCacheTest, Remove)
{
    AssetId id = GenerateAssetId();
    (void)cache.FindOrCreate(id, TypeId::Get<MockTexture>(), AssetPath("test.png"));

    EXPECT_EQ(cache.GetCount(), 1u);

    cache.Remove(id);
    EXPECT_EQ(cache.GetCount(), 0u);
    EXPECT_FALSE(cache.Find(id).HasValue());
}

TEST_F(AssetCacheTest, CollectGarbage_RemovesUnused)
{
    AssetId id1 = GenerateAssetId();
    AssetId id2 = GenerateAssetId();

    HandleData hd1 = cache.FindOrCreate(id1, TypeId::Get<MockTexture>(), AssetPath("tex1.png"));
    HandleData hd2 = cache.FindOrCreate(id2, TypeId::Get<MockTexture>(), AssetPath("tex2.png"));

    EXPECT_EQ(cache.GetCount(), 2u);

    // slot1에 strong handle 유지, slot2에는 없음
    cache.GetTable().GetSlot(hd1.index).strong_count.fetch_add(1, std::memory_order_relaxed);

    uint32 removed = cache.CollectGarbage();
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(cache.GetCount(), 1u);

    // slot1 still exists
    EXPECT_TRUE(cache.Find(id1).HasValue());
    EXPECT_FALSE(cache.Find(id2).HasValue());

    // cleanup
    cache.GetTable().GetSlot(hd1.index).strong_count.fetch_sub(1, std::memory_order_relaxed);
}

TEST_F(AssetCacheTest, CollectGarbage_KeepsInUse)
{
    AssetId id = GenerateAssetId();
    HandleData hd = cache.FindOrCreate(id, TypeId::Get<MockTexture>(), AssetPath("test.png"));

    // strong_count를 1로 설정하여 사용 중으로 표시
    cache.GetTable().GetSlot(hd.index).strong_count.fetch_add(1, std::memory_order_relaxed);

    uint32 removed = cache.CollectGarbage();
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(cache.GetCount(), 1u);

    // cleanup
    cache.GetTable().GetSlot(hd.index).strong_count.fetch_sub(1, std::memory_order_relaxed);
}

// =============================================================================
// AssetHandle Tests
// =============================================================================

class AssetHandleTest : public ::testing::Test
{
protected:
    AssetPool cache;

    /** HandleTable에 슬롯을 만들고 MockTexture를 로드한 HandleData를 반환합니다. */
    HandleData CreateSlotWithAsset(const AssetId& id)
    {
        HandleData hd = cache.FindOrCreate(id, TypeId::Get<MockTexture>(), AssetPath("test.png"));
        SlotEntry& slot = cache.GetTable().GetSlot(hd.index);

        auto* texture = new MockTexture();
        texture->width = 512;
        slot.destructor = [](void* p) { delete static_cast<MockTexture*>(p); };
        (void)slot.ExchangeAsset(texture);
        slot.SetState(ELoadingState::Loaded);

        return hd;
    }
};

TEST_F(AssetHandleTest, DefaultConstruction)
{
    AssetHandle<MockTexture> handle;

    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(handle.Get(), nullptr);
}

TEST_F(AssetHandleTest, ConstructionWithHandleData)
{
    AssetId id = GenerateAssetId();
    HandleData hd = CreateSlotWithAsset(id);

    AssetHandle<MockTexture> handle(hd, &cache.GetTable());

    EXPECT_TRUE(handle.IsValid());
    EXPECT_NE(handle.Get(), nullptr);
    EXPECT_EQ(handle.GetAssetId(), id);
}

TEST_F(AssetHandleTest, Get_RawPointer)
{
    AssetId id = GenerateAssetId();
    HandleData hd = CreateSlotWithAsset(id);

    AssetHandle<MockTexture> handle(hd, &cache.GetTable());

    MockTexture* ptr = handle.Get();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->width, 512);
}

TEST_F(AssetHandleTest, OperatorArrow)
{
    AssetId id = GenerateAssetId();
    HandleData hd = CreateSlotWithAsset(id);

    AssetHandle<MockTexture> handle(hd, &cache.GetTable());

    EXPECT_EQ(handle->width, 512);
    handle->width = 1024;
    EXPECT_EQ(handle->width, 1024);
}

TEST_F(AssetHandleTest, OperatorBool)
{
    AssetHandle<MockTexture> invalid_handle;
    EXPECT_FALSE(static_cast<bool>(invalid_handle));

    AssetId id = GenerateAssetId();
    HandleData hd = CreateSlotWithAsset(id);
    AssetHandle<MockTexture> valid_handle(hd, &cache.GetTable());
    EXPECT_TRUE(static_cast<bool>(valid_handle));
}

TEST_F(AssetHandleTest, CopyConstruction)
{
    AssetId id = GenerateAssetId();
    HandleData hd = CreateSlotWithAsset(id);

    AssetHandle<MockTexture> handle1(hd, &cache.GetTable());
    AssetHandle<MockTexture> handle2(handle1);

    EXPECT_TRUE(handle2.IsValid());
    EXPECT_EQ(handle1.Get(), handle2.Get());
    EXPECT_EQ(handle1.GetAssetId(), handle2.GetAssetId());

    // strong_count가 2여야 함 (handle1 + handle2)
    EXPECT_EQ(cache.GetTable().GetSlot(hd.index).strong_count.load(), 2u);
}

TEST_F(AssetHandleTest, MoveConstruction)
{
    AssetId id = GenerateAssetId();
    HandleData hd = CreateSlotWithAsset(id);

    AssetHandle<MockTexture> handle1(hd, &cache.GetTable());
    AssetHandle<MockTexture> handle2(std::move(handle1));

    EXPECT_TRUE(handle2.IsValid());
    EXPECT_EQ(handle2.GetAssetId(), id);

    // strong_count가 1이어야 함 (handle2만)
    EXPECT_EQ(cache.GetTable().GetSlot(hd.index).strong_count.load(), 1u);
}

TEST_F(AssetHandleTest, InvalidHandle_ReturnsNull)
{
    AssetHandle<MockTexture> handle;

    EXPECT_EQ(handle.Get(), nullptr);
    EXPECT_EQ(handle.GetAssetId(), AssetId::Invalid);
}

TEST_F(AssetHandleTest, StrongCount_DecrementOnDestruct)
{
    AssetId id = GenerateAssetId();
    HandleData hd = CreateSlotWithAsset(id);

    {
        AssetHandle<MockTexture> handle(hd, &cache.GetTable());
        EXPECT_EQ(cache.GetTable().GetSlot(hd.index).strong_count.load(), 1u);
    }
    // handle이 소멸되면 strong_count가 0이 되어야 함
    EXPECT_EQ(cache.GetTable().GetSlot(hd.index).strong_count.load(), 0u);
}

// =============================================================================
// AssetRegistry Tests
// =============================================================================

class AssetRegistryTest : public ::testing::Test
{
protected:
    static AssetMetadata CreateDummyMeta()
    {
        AssetMetadata meta;
        meta.guid = Guid::NewGuid();
        meta.cache_version = 1;
        return meta;
    }

protected:
    AssetRegistry registry;
};

TEST_F(AssetRegistryTest, RegisterAsset)
{
    AssetId id = GenerateAssetId();
    TypeId type = TypeId::Get<MockTexture>();
    AssetPath path("textures/diffuse.png");

    registry.RegisterAsset(id, type, path, CreateDummyMeta());

    auto retrieved_id = registry.GetAssetId(path);
    ASSERT_TRUE(retrieved_id.HasValue());
    EXPECT_EQ(retrieved_id.Value(), id);

    // Then 2: AssetId로 레코드 전체 조회 (Visitor 패턴 검증)
    bool is_record_found = registry.ReadRecord(id, [&](const AssetRecord& record)
    {
        // 콜백 내부에서 안전하게 모든 프로퍼티를 한 번에 검증할 수 있습니다.
        EXPECT_EQ(record.id, id);
        EXPECT_EQ(record.type, type);
        EXPECT_EQ(record.logical_path, path);

        // 메타데이터 내부의 값도 여기서 검증 가능합니다.
        // EXPECT_EQ(record.metadata.source_hash, "...");
    });

    // ReadRecord 자체가 성공적으로 콜백을 호출했는지 검증
    EXPECT_TRUE(is_record_found);
}

TEST_F(AssetRegistryTest, GetAssetId_NonExistent)
{
    AssetPath path("non/existent.png");
    auto result = registry.GetAssetId(path);

    EXPECT_FALSE(result.HasValue());
}

TEST_F(AssetRegistryTest, GetAssetPath_NonExistent)
{
    AssetId id = GenerateAssetId(); // 등록되지 않은 랜덤 ID

    bool is_record_found = registry.ReadRecord(id, [](const AssetRecord& /*record*/)
    {
        // 이 콜백은 절대 실행되어서는 안 됩니다.
        FAIL() << "Callback should not be invoked for a non-existent AssetId.";
    });

    EXPECT_FALSE(is_record_found);
}

TEST_F(AssetRegistryTest, MultipleAssetsInSameFile)
{
    VPath file_path("models/character.fbx");
    AssetId id1 = GenerateAssetId();
    AssetId id2 = GenerateAssetId();
    TypeId mesh_type = TypeId::Get<MockMesh>();

    AssetPath path1(file_path, "Mesh_01");
    AssetPath path2(file_path, "Mesh_02");

    registry.RegisterAsset(id1, mesh_type, AssetPath(path1), CreateDummyMeta());
    registry.RegisterAsset(id2, mesh_type, AssetPath(path2), CreateDummyMeta());

    EXPECT_TRUE(registry.GetAssetId(path1).HasValue());
    EXPECT_TRUE(registry.GetAssetId(path2).HasValue());
    EXPECT_NE(registry.GetAssetId(path1).Value(), registry.GetAssetId(path2).Value());

    auto assets = registry.GetAssetsInFile(file_path);
    EXPECT_EQ(assets.Len(), 2u);
}

TEST_F(AssetRegistryTest, IsFileImported_AutoTracking)
{
    VPath file_path("textures/test.png");
    AssetPath asset_path{ file_path, {} };

    EXPECT_FALSE(registry.IsFileImported(file_path));

    registry.RegisterAsset(GenerateAssetId(), TypeId::Get<MockTexture>(), asset_path, CreateDummyMeta());

    // 보조 인덱스(file_to_assets)가 정상적으로 동작하여 true 반환하는지 확인
    EXPECT_TRUE(registry.IsFileImported(file_path));
}

TEST_F(AssetRegistryTest, FindFirstOfType)
{
    VPath file_path("models/multi.fbx");
    AssetId mesh_id = GenerateAssetId();
    AssetId texture_id = GenerateAssetId();

    registry.RegisterAsset(mesh_id, TypeId::Get<MockMesh>(), AssetPath(file_path, "MainMesh"), CreateDummyMeta());
    registry.RegisterAsset(texture_id, TypeId::Get<MockTexture>(), AssetPath(file_path, "Texture"), CreateDummyMeta());

    auto found_mesh = registry.FindFirstOfType(file_path, TypeId::Get<MockMesh>());
    ASSERT_TRUE(found_mesh.HasValue());
    EXPECT_EQ(found_mesh.Value(), mesh_id);

    auto found_texture = registry.FindFirstOfType(file_path, TypeId::Get<MockTexture>());
    ASSERT_TRUE(found_texture.HasValue());
    EXPECT_EQ(found_texture.Value(), texture_id);
}

TEST_F(AssetRegistryTest, GetAssetsInFile_Empty)
{
    VPath file_path("empty/file.fbx");

    auto assets = registry.GetAssetsInFile(file_path);
    EXPECT_TRUE(assets.IsEmpty());
}

// =============================================================================
// Integration Tests
// =============================================================================

class AssetManagementIntegrationTest : public ::testing::Test
{
protected:
    static AssetMetadata CreateDummyMeta()
    {
        AssetMetadata meta;
        meta.guid = Guid::NewGuid();
        return meta;
    }

protected:
    AssetPool cache;
    AssetRegistry registry;
};

TEST_F(AssetManagementIntegrationTest, FullWorkflow)
{
    // 1. Register asset in registry
    AssetId id = GenerateAssetId();
    TypeId type = TypeId::Get<MockTexture>();
    AssetPath path("textures/diffuse.png");

    registry.RegisterAsset(id, type, path, CreateDummyMeta());

    // 2. Create slot in cache
    HandleData hd = cache.FindOrCreate(id, type, path);
    SlotEntry& slot = cache.GetTable().GetSlot(hd.index);

    // 3. Load asset into slot
    auto* texture = new MockTexture();
    texture->width = 2048;
    slot.destructor = [](void* p) { delete static_cast<MockTexture*>(p); };
    (void)slot.ExchangeAsset(texture);
    slot.SetState(ELoadingState::Loaded);

    // 4. Create handle
    AssetHandle<MockTexture> handle(hd, &cache.GetTable());

    // 5. Use handle
    ASSERT_TRUE(handle.IsValid());
    EXPECT_EQ(handle->width, 2048);

    // 6. Verify registry
    auto retrieved_id = registry.GetAssetId(path);
    ASSERT_TRUE(retrieved_id.HasValue());
    EXPECT_EQ(retrieved_id.Value(), id);
}

TEST_F(AssetManagementIntegrationTest, MultipleHandlesToSameAsset)
{
    AssetId id = GenerateAssetId();
    HandleData hd = cache.FindOrCreate(id, TypeId::Get<MockMesh>(), AssetPath("mesh.obj"));
    SlotEntry& slot = cache.GetTable().GetSlot(hd.index);

    auto* mesh = new MockMesh();
    mesh->vertex_count = 500;
    slot.destructor = [](void* p) { delete static_cast<MockMesh*>(p); };
    (void)slot.ExchangeAsset(mesh);
    slot.SetState(ELoadingState::Loaded);

    AssetHandle<MockMesh> handle1(hd, &cache.GetTable());
    AssetHandle<MockMesh> handle2(hd, &cache.GetTable());
    AssetHandle<MockMesh> handle3(handle1);

    EXPECT_EQ(handle1.Get(), handle2.Get());
    EXPECT_EQ(handle1.Get(), handle3.Get());
    EXPECT_EQ(handle1->vertex_count, 500);
    EXPECT_EQ(handle2->vertex_count, 500);
    EXPECT_EQ(handle3->vertex_count, 500);

    // strong_count: handle1 + handle2 + handle3 = 3
    EXPECT_EQ(slot.strong_count.load(), 3u);
}

TEST_F(AssetManagementIntegrationTest, SubAssetHandling)
{
    VPath file_path("models/character.fbx");

    AssetId main_mesh_id = GenerateAssetId();
    AssetId weapon_mesh_id = GenerateAssetId();

    AssetPath main_path(file_path, "MainBody");
    AssetPath weapon_path(file_path, "Weapon");

    TypeId mesh_type = TypeId::Get<MockMesh>();

    // Register both sub-assets with metadata
    registry.RegisterAsset(main_mesh_id, mesh_type, AssetPath(main_path), CreateDummyMeta());
    registry.RegisterAsset(weapon_mesh_id, mesh_type, AssetPath(weapon_path), CreateDummyMeta());

    // Create slots
    HandleData main_hd = cache.FindOrCreate(main_mesh_id, mesh_type, main_path);
    HandleData weapon_hd = cache.FindOrCreate(weapon_mesh_id, mesh_type, weapon_path);

    SlotEntry& main_slot = cache.GetTable().GetSlot(main_hd.index);
    SlotEntry& weapon_slot = cache.GetTable().GetSlot(weapon_hd.index);

    // Load assets
    auto* main_mesh = new MockMesh();
    main_mesh->vertex_count = 1000;
    main_slot.destructor = [](void* p) { delete static_cast<MockMesh*>(p); };
    (void)main_slot.ExchangeAsset(main_mesh);
    main_slot.SetState(ELoadingState::Loaded);

    auto* weapon_mesh = new MockMesh();
    weapon_mesh->vertex_count = 200;
    weapon_slot.destructor = [](void* p) { delete static_cast<MockMesh*>(p); };
    (void)weapon_slot.ExchangeAsset(weapon_mesh);
    weapon_slot.SetState(ELoadingState::Loaded);

    // Verify
    EXPECT_TRUE(registry.IsFileImported(file_path));

    auto assets = registry.GetAssetsInFile(file_path);
    EXPECT_EQ(assets.Len(), 2u);

    AssetHandle<MockMesh> main_handle(main_hd, &cache.GetTable());
    AssetHandle<MockMesh> weapon_handle(weapon_hd, &cache.GetTable());

    EXPECT_EQ(main_handle->vertex_count, 1000);
    EXPECT_EQ(weapon_handle->vertex_count, 200);
}
