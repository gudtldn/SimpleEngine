#pragma once
#include <mutex>

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/AssetSlot.h"
#include "SimpleEngine/Asset/Pipeline/AssetImporter.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
// forward declaration
class AssetCache;
class AssetRegistry;
class DerivedDataCache;
class AssetSubsystem;

// DDC Miss Handler
using DDCMissHandler = Function<bool(AssetSubsystem& subsystem, const Path& file_path)>;

/**
 * Asset 로딩, 캐싱, DDC 통합을 관리하는 Core 서브시스템
 *
 * 로딩 흐름:
 *   1. Registry에서 AssetId 조회
 *   2. AssetCache(메모리) Hit -> 즉시 반환
 *   3. DDC Hit (source_hash/cache_version 일치) -> 역직렬화 -> Cache 적재 -> 반환
 *   4. DDC Miss -> Import 파이프라인 실행 -> DDC에 저장 -> Cache 적재 -> 반환
 *   5. Registry 미등록 (런타임 fallback) -> ImportAndRegisterAll
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) AssetSubsystem : public SubsystemBase
{
    SE_CLASS(AssetSubsystem, SubsystemBase)

public:
    AssetSubsystem();
    virtual ~AssetSubsystem() override;

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

public:
    /**
     * 지정된 경로의 Asset을 로드하고 Handle을 반환합니다.
     * @param asset_path Asset 경로 (예: "meshes/model.fbx#Mesh_01")
     */
    template <typename T>
        requires std::derived_from<T, AssetBase>
    [[nodiscard]] AssetHandle<T> Load(const AssetPath& asset_path);

    /**
     * 캐시에서 Asset을 찾습니다. (Import 수행 안함)
     * @param asset_id Asset의 고유 ID
     */
    template <typename T>
        requires std::derived_from<T, AssetBase>
    [[nodiscard]] AssetHandle<T> Find(const AssetId& asset_id) const;

    /** Asset을 프레임 마지막에 안전하게 해제할 수 있도록 대기 큐(Pending Queue)에 삽입합니다. */
    void DeferRelease(std::shared_ptr<AssetBase> asset);

    /** 프레임 끝에서 대기 큐(Pending Queue)를 정리합니다. */
    void EndFrame();

public:
    /** Asset을 DDC payload로 직렬화합니다. */
    [[nodiscard]] static Array<uint8> SerializeAssetPayload(const AssetBase& asset);

    /** DDC payload에서 Asset을 역직렬화합니다. */
    [[nodiscard]] static std::shared_ptr<AssetBase> DeserializeAssetPayload(const TypeId& type_id, const Array<uint8>& payload);

public:
    [[nodiscard]] FORCE_INLINE AssetImporter& GetImporter() const { return *importer; }
    [[nodiscard]] FORCE_INLINE AssetCache& GetCache() const { return *cache; }
    [[nodiscard]] FORCE_INLINE AssetRegistry& GetRegistry() const { return *registry; }
    [[nodiscard]] FORCE_INLINE DerivedDataCache& GetDDC() const { return *ddc; }

private:
    [[nodiscard]] std::shared_ptr<AssetSlot> LoadInternal(const TypeId& expected_type, const AssetPath& source_path);
    [[nodiscard]] std::shared_ptr<AssetSlot> FindInternal(const TypeId& expected_type, const AssetId& asset_id) const;

    /** 파일 Import 후 모든 Sub-Asset을 캐시에 등록합니다. */
    [[deprecated("Use EditorAssetSubsystem for importing. This will be removed soon.")]]
    bool ImportAndRegisterAll(const Path& file_path);

private:
    std::unique_ptr<AssetImporter> importer;
    std::unique_ptr<AssetCache> cache;
    std::unique_ptr<AssetRegistry> registry;
    std::unique_ptr<DerivedDataCache> ddc;

    // Deferred Release
    TracyLockable(std::mutex, pending_mutex);
    Array<std::shared_ptr<AssetBase>> pending_release;
};

template <typename T>
    requires std::derived_from<T, AssetBase>
AssetHandle<T> AssetSubsystem::Load(const AssetPath& asset_path)
{
    std::shared_ptr<AssetSlot> slot = LoadInternal(TypeId::Get<T>(), asset_path);
    return AssetHandle<T>{ std::move(slot) };
}

template <typename T>
    requires std::derived_from<T, AssetBase>
AssetHandle<T> AssetSubsystem::Find(const AssetId& asset_id) const
{
    std::shared_ptr<AssetSlot> slot = FindInternal(TypeId::Get<T>(), asset_id);
    return AssetHandle<T>{ std::move(slot) };
}
}  // namespace se::asset
