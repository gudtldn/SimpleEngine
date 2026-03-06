#pragma once

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/AssetPayload.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "tracy/Tracy.hpp"

#include <mutex>


namespace se::asset
{
// forward declaration
class AssetPool;
class AssetRegistry;
class DerivedDataCache;
class AssetSubsystem;

// DDC Miss Handler
using DDCMissHandler = Function<bool(AssetSubsystem& subsystem, const VPath& file_path)>;

/**
 * Asset 로딩, 캐싱, DDC 통합을 관리하는 Core 서브시스템
 *
 * 로딩 흐름:
 *   1. Registry에서 AssetId 조회
 *   2. AssetPool(메모리) Hit -> 즉시 반환
 *   3. DDC Hit (source_hash/cache_version 일치) -> 역직렬화 -> Pool 적재 -> 반환
 *   4. DDC Miss -> (Editor) Import 파이프라인 실행 -> DDC에 저장 -> Pool 적재 -> 반환
 *   5. DDC Miss Handler 미등록 (런타임 fallback) -> Invalid Handle
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
    /** DDC Miss 시 호출될 핸들러를 등록합니다. */
    void SetDDCMissHandler(DDCMissHandler handler);

    /**
     * 지정된 경로의 Asset을 로드하고 Handle을 반환합니다.
     * @param asset_path Asset 경로 (예: "meshes/model.fbx#Mesh_01")
     * @param scope 에셋의 수명 범위 및 관리 우선순위 (기본값: Scene)
     */
    template <typename T>
        requires std::derived_from<T, AssetBase>
    [[nodiscard]] AssetHandle<T> Load(const AssetPath& asset_path, EScopeLayer scope = EScopeLayer::Scene);

    /**
     * 캐시에서 Asset을 찾습니다. (Import 수행 안함)
     * @param asset_id Asset의 고유 ID
     */
    template <typename T>
        requires std::derived_from<T, AssetBase>
    [[nodiscard]] AssetHandle<T> Find(const AssetId& asset_id) const;

    /** Asset payload를 프레임 마지막에 안전하게 해제할 수 있도록 대기 큐(Pending Queue)에 삽입합니다. */
    void DeferRelease(AssetPayload payload);

    /** 프레임 끝에서 대기 큐(Pending Queue)를 정리합니다. */
    void EndFrame();

public:
    /** Asset을 DDC payload로 직렬화합니다. */
    [[nodiscard]] static Array<uint8> SerializeAssetPayload(const AssetBase& asset);

    /**
     * DDC payload에서 Asset을 역직렬화하여 AssetPayload로 반환합니다.
     * ptr과 destructor가 분리된 상태로 반환되므로, SlotEntry에 직접 저장할 수 있습니다.
     */
    [[nodiscard]] static AssetPayload DeserializeAssetPayload(const TypeId& type_id, const Array<uint8>& payload);

public:
    [[nodiscard]] FORCE_INLINE AssetPool& GetPool() const { return *pool; }
    [[nodiscard]] FORCE_INLINE AssetRegistry& GetRegistry() const { return *registry; }
    [[nodiscard]] FORCE_INLINE DerivedDataCache& GetDDC() const { return *ddc; }

private:
    [[nodiscard]] HandleData LoadInternal(const TypeId& expected_type, const AssetPath& source_path, EScopeLayer scope);
    [[nodiscard]] HandleData FindInternal(const TypeId& expected_type, const AssetId& asset_id) const;
    [[nodiscard]] HandleTable& GetHandleTable() const;

private:
    std::unique_ptr<AssetPool> pool;
    std::unique_ptr<AssetRegistry> registry;
    std::unique_ptr<DerivedDataCache> ddc;

    DDCMissHandler ddc_miss_handler;

    // Frame counter (Eviction 정책용)
    uint64 frame_count = 0;

    TracyLockable(std::mutex, loading_mutex);
    std::condition_variable_any import_cv;    // 하나의 스레드에서만 Import를 보장하는 cv
    HashSet<VPath> files_currently_importing; // 현재 Import 중인 File 목록
};

template <typename T>
    requires std::derived_from<T, AssetBase>
AssetHandle<T> AssetSubsystem::Load(const AssetPath& asset_path, EScopeLayer scope)
{
    if (HandleData handle_data = LoadInternal(TypeId::Get<T>(), asset_path, scope))
    {
        return AssetHandle<T>{ handle_data, &GetHandleTable() };
    }
    return {};
}

template <typename T>
    requires std::derived_from<T, AssetBase>
AssetHandle<T> AssetSubsystem::Find(const AssetId& asset_id) const
{
    if (HandleData handle_data = FindInternal(TypeId::Get<T>(), asset_id))
    {
        return AssetHandle<T>{ handle_data, &GetHandleTable() };
    }
    return {};
}
}  // namespace se::asset
