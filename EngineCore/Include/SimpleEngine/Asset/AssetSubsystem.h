#pragma once
#include <mutex>

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/AssetSlot.h"
#include "SimpleEngine/Asset/Pipeline/AssetImporter.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
class AssetCache;
class AssetRegistry;

/**
 * @todo docs
 */
class SE_CORE_API AssetSubsystem : public ISubsystem
{
public:
    AssetSubsystem();
    virtual ~AssetSubsystem() override;

public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem

public:
    /**
     * 지정된 경로의 Asset을 로드하고 Handle을 반환합니다.
     * @param asset_path Asset 경로 (예: "meshes/model.fbx#Mesh_01")
     */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] AssetHandle<T> Load(const AssetPath& asset_path);

    /**
     * 캐시에서 Asset을 찾습니다. (Import 수행 안함)
     * @param asset_id Asset의 고유 ID
     */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] AssetHandle<T> Find(const AssetId& asset_id) const;

    /** Asset을 프레임 마지막에 안전하게 해제할 수 있도록 대기 큐(Pending Queue)에 삽입합니다. */
    void DeferRelease(std::shared_ptr<IAsset> asset);

    /** 프레임 끝에서 대기 큐(Pending Queue)를 정리합니다. */
    void EndFrame();

public:
    [[nodiscard]] FORCE_INLINE AssetImporter& GetImporter() const { return *importer; }
    [[nodiscard]] FORCE_INLINE AssetCache& GetCache() const { return *cache; }

private:
    [[nodiscard]] std::shared_ptr<AssetSlot> LoadInternal(const TypeId& expected_type, const AssetPath& source_path);
    [[nodiscard]] std::shared_ptr<AssetSlot> FindInternal(const TypeId& expected_type, const AssetId& asset_id) const;

    /** 파일 Import 후 모든 Sub-Asset을 캐시에 등록합니다. */
    bool ImportAndRegisterAll(const Path& file_path);

private:
    std::unique_ptr<AssetImporter> importer;
    std::unique_ptr<AssetCache> cache;
    std::unique_ptr<AssetRegistry> registry;

    // Deferred Release
    TracyLockable(std::mutex, pending_mutex);
    Array<std::shared_ptr<IAsset>> pending_release;
};

template <typename T>
    requires std::derived_from<T, IAsset>
AssetHandle<T> AssetSubsystem::Load(const AssetPath& asset_path)
{
    std::shared_ptr<AssetSlot> slot = LoadInternal(TypeId::Get<T>(), asset_path);
    return AssetHandle<T>{ std::move(slot) };
}

template <typename T>
    requires std::derived_from<T, IAsset>
AssetHandle<T> AssetSubsystem::Find(const AssetId& asset_id) const
{
    std::shared_ptr<AssetSlot> slot = FindInternal(TypeId::Get<T>(), asset_id);
    return AssetHandle<T>{ std::move(slot) };
}
}  // namespace se::asset
