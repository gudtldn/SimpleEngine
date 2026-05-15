#pragma once

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/AssetPayload.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/JobTask.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "tracy/Tracy.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>


namespace se
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
     * 지정된 경로의 Asset을 비동기로 로드합니다.
     *
     * DDC hit 시 AsyncFileIO를 통해 비동기 I/O를 수행합니다.
     * DDC miss 시 워커 스레드에서 동기 LoadInternal()로 fallback합니다.
     * 어느 경우든 호출 스레드는 블로킹되지 않습니다.
     *
     * @param asset_path Asset 경로 (예: "meshes/model.fbx#Mesh_01")
     * @param scope 에셋의 수명 범위 및 관리 우선순위 (기본값: Scene)
     * @return co_await로 대기 가능한 JobTask. 완료 시 AssetHandle<T>를 반환합니다.
     */
    template <typename T>
        requires std::derived_from<T, AssetBase>
    [[nodiscard]] JobTask<AssetHandle<T>> LoadAsync(AssetPath asset_path, EScopeLayer scope = EScopeLayer::Scene);

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

    /**
     * Built-In 에셋을 Global scope로 영구 등록합니다.
     * @param asset_id 등록할 well-known AssetId
     * @param asset 등록할 에셋 인스턴스
     */
    template <typename T>
        requires std::derived_from<T, AssetBase>
    [[nodiscard]] AssetHandle<T> RegisterBuiltin(const AssetId& asset_id, std::unique_ptr<T> asset);

public:
    /** Asset을 DDC payload로 직렬화합니다. */
    [[nodiscard]] static Array<u8> SerializeAssetPayload(const AssetBase& asset);

    /**
     * DDC payload에서 Asset을 역직렬화하여 AssetPayload로 반환합니다.
     * ptr과 destructor가 분리된 상태로 반환되므로, SlotEntry에 직접 저장할 수 있습니다.
     */
    [[nodiscard]] static AssetPayload DeserializeAssetPayload(const TypeId& type_id, ArrayView<const u8> payload_view);

public:
    [[nodiscard]] FORCE_INLINE AssetPool& GetPool() const { return *pool; }
    [[nodiscard]] FORCE_INLINE AssetRegistry& GetRegistry() const { return *registry; }
    [[nodiscard]] FORCE_INLINE DerivedDataCache& GetDDC() const { return *ddc; }

private:
    enum class ESlotAcquireResult : u8
    {
        Loaded,   // 메모리 Cache Hit (handle_data를 즉시 반환 가능)
        Acquired, // BeginLoad 획득 성공 (DDC 읽기 진행)
        Failed,   // 타입 불일치
    };

    [[nodiscard]] HandleData LoadInternal(const TypeId& expected_type, const AssetPath& source_path, EScopeLayer scope);
    [[nodiscard]] JobTask<HandleData> LoadAsyncInternal(TypeId expected_type, AssetPath source_path, EScopeLayer scope);
    [[nodiscard]] HandleData FindInternal(const TypeId& expected_type, const AssetId& asset_id) const;
    [[nodiscard]] HandleTable& GetHandleTable() const;
    [[nodiscard]] HandleData RegisterBuiltinInternal(const AssetId& asset_id, const TypeId& type_id, AssetPayload payload, u64 asset_size);

    /**
     * 슬롯 상태를 확인하고 로딩 권한(BeginLoad)을 획득합니다.
     * @warning WaitForLoadComplete()가 블로킹이므로 코루틴 컨텍스트에서 호출 시 워커 스레드를 점유합니다.
     */
    [[nodiscard]] ESlotAcquireResult AcquireLoadSlot(HandleData handle_data, const TypeId& expected_type);

    /** 역직렬화된 payload를 SlotEntry에 커밋합니다. (메모리 추적 + 상태 전환 + 구 payload 지연 해제) */
    void CommitLoadedPayload(HandleData handle_data, AssetPayload payload, u64 payload_size, EScopeLayer scope);

private:
    std::unique_ptr<AssetPool> pool;
    std::unique_ptr<AssetRegistry> registry;
    std::unique_ptr<DerivedDataCache> ddc;

    DDCMissHandler ddc_miss_handler;

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
JobTask<AssetHandle<T>> AssetSubsystem::LoadAsync(AssetPath asset_path, EScopeLayer scope)
{
    if (HandleData handle_data = co_await LoadAsyncInternal(TypeId::Get<T>(), std::move(asset_path), scope))
    {
        co_return AssetHandle<T>{ handle_data, &GetHandleTable() };
    }
    co_return {};
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

template <typename T>
    requires std::derived_from<T, AssetBase>
AssetHandle<T> AssetSubsystem::RegisterBuiltin(const AssetId& asset_id, std::unique_ptr<T> asset)
{
    const AssetPayload payload = {
        .ptr = asset.get(),
        .destructor = [](void* p) noexcept { delete static_cast<T*>(p); },
    };

    if (HandleData handle_data = RegisterBuiltinInternal(asset_id, TypeId::Get<T>(), payload, sizeof(T)))
    {
        asset.release();
        return AssetHandle<T>{ handle_data, &GetHandleTable() };
    }
    return {};
}
} // namespace se
