#pragma once
#include <memory>
#include <shared_mutex>
#include <atomic>

#include "AssetId.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Asset/Types/IAsset.h"

#include "tracy/Tracy.hpp"


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
class AssetSlot
{
public:
    explicit AssetSlot(AssetId id, Path path, const TypeId& type_id);

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
    [[nodiscard]] IAsset* GetRawAsset() const;

    /**
     * 현재 로드된 에셋을 가져옵니다.
     * 로딩 중이거나 실패했다면 nullptr을 반환할 수 있습니다.
     */
    [[nodiscard]] std::shared_ptr<IAsset> GetAsset() const;

    /** 에셋 데이터를 교체합니다. */
    void SetAsset(std::shared_ptr<IAsset> new_asset, ELoadingState new_state = ELoadingState::Loaded);

    /** 현재 에셋의 로딩 상태를 가져옵니다.*/
    [[nodiscard]] ELoadingState GetState() const;

    /** 현재 에셋의 로딩 상태를 설정합니다.*/
    void SetState(ELoadingState state);

    /** 이 Slot의 소유하는 Asset의 고유 ID를 반환합니다. */
    [[nodiscard]] FORCE_INLINE AssetId GetAssetId() const { return asset_id; }

    /** 이 Slot이 소유하는 Asset의 타입을 반환합니다. */
    [[nodiscard]] FORCE_INLINE TypeId GetAssetType() const { return asset_type; }

    /** 이 Slot이 소유하는 Asset의 실제 물리적 위치를 반환합니다. */
    [[nodiscard]] FORCE_INLINE const Path& GetSourcePath() const { return source_path; }

    /** 에셋을 무효화 합니다. */
    void Invalidate();

private:
    mutable TracySharedLockable(std::shared_mutex, mutex);
    std::shared_ptr<IAsset> asset;

    // Asset Metadata
    const AssetId asset_id;
    const TypeId asset_type;
    Path source_path;
    std::atomic<ELoadingState> loading_state;
};
} // namespace se::asset
