#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"

#include "tracy/Tracy.hpp"

#include <atomic>
#include <memory>
#include <shared_mutex>


namespace se::asset
{
/**
 * Asset의 로딩 상태
 */
enum class ELoadingState : uint8
{
    Unloaded, // 데이터 없음 (초기 상태)
    Loading,  // 로딩 중 (Placeholder)
    Loaded,   // 사용 가능
    Failed,   // 로딩 실패
};

/**
 * Asset의 상태와 데이터를 관리하는 제어 블록 클래스
 */
class SE_CORE_API AssetSlot
{
public:
    explicit AssetSlot(const AssetId& id, const TypeId& type_id, AssetPath path);

    // 복사 & 이동 금지
    AssetSlot(const AssetSlot&) = delete;
    AssetSlot& operator=(const AssetSlot&) = delete;
    AssetSlot(AssetSlot&&) = delete;
    AssetSlot& operator=(AssetSlot&&) = delete;

public:
    /**
     * 현재 로드된 에셋을 Raw Pointer로 가져옵니다.
     * 로딩 중이거나 실패했다면 nullptr을 반환할 수 있습니다.
     */
    [[nodiscard]] AssetBase* GetRawAsset() const;

    /**
     * 현재 로드된 에셋을 가져옵니다.
     * 로딩 중이거나 실패했다면 nullptr을 반환할 수 있습니다.
     */
    [[nodiscard]] std::shared_ptr<AssetBase> GetAsset() const;

    /** 에셋 데이터를 교체 후, 이전 에셋을 반환합니다. */
    [[nodiscard]] std::shared_ptr<AssetBase> ExchangeAsset(std::shared_ptr<AssetBase> new_asset, ELoadingState new_state = ELoadingState::Loaded);

    /** 현재 에셋의 로딩 상태를 가져옵니다.*/
    [[nodiscard]] ELoadingState GetState() const;

    /** 현재 에셋의 로딩 상태를 설정합니다.*/
    void SetState(ELoadingState state);

    /** 로딩 권한을 원자적으로 획득합니다. (Unloaded/Failed -> Loading) */
    [[nodiscard]] bool BeginLoad();

    /** 만약 상태가 Loading인 경우 완료(Loaded 또는 Failed)될 때까지 대기합니다. */
    void WaitForLoadComplete() const;

    /** 이 Slot의 소유하는 Asset의 고유 ID를 반환합니다. */
    [[nodiscard]] FORCE_INLINE AssetId GetAssetId() const { return asset_id; }

    /** 이 Slot이 소유하는 Asset의 타입을 반환합니다. */
    [[nodiscard]] FORCE_INLINE TypeId GetAssetType() const { return asset_type; }

    /** 이 Slot이 소유하는 Asset의 실제 물리적 위치를 반환합니다. */
    [[nodiscard]] FORCE_INLINE const AssetPath& GetSourcePath() const { return source_path; }

    /** 에셋을 무효화 후, 이전 에셋을 반환합니다. */
    [[nodiscard]] std::shared_ptr<AssetBase> Invalidate();

private:
    mutable TracySharedLockable(std::shared_mutex, mutex);
    std::shared_ptr<AssetBase> asset;

    // lock-free 읽기 전용 포인터
    std::atomic<AssetBase*> cached_asset;

    // Asset Metadata
    const AssetId asset_id;
    const TypeId asset_type;
    AssetPath source_path;
    std::atomic<ELoadingState> loading_state;
};
} // namespace se::asset
