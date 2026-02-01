#pragma once
#include <concepts>
#include <memory>
#include <utility>

#include "SimpleEngine/Asset/AssetSlot.h"
#include "SimpleEngine/Asset/Types/IAsset.h"


namespace se::asset
{
/**
 * AssetSlot을 가리키는 경량 핸들 클래스
 * 엔진의 생명주기 관리(Frame-based GC)를 통해 Raw Pointer 접근의 안전성을 보장받습니다.
 * @tparam T 핸들이 참조하는 Asset의 타입 (ex: Texture, Material, Mesh)
 */
template <typename T>
    requires std::derived_from<T, IAsset>
class AssetHandle
{
public:
    using AssetType = T;

public:
    AssetHandle() = default;

    explicit AssetHandle(std::shared_ptr<AssetSlot> in_slot)
        : slot(std::move(in_slot))
    {
    }

public:
    /**
     * 에셋의 Raw Pointer를 반환합니다. (High Performance)
     * @return 유효한 에셋 포인터 또는 nullptr
     */
    [[nodiscard]] T* Get() const
    {
        if (slot)
        {
            IAsset* asset_ptr = slot->GetRawAsset();
            return static_cast<T*>(asset_ptr);
        }
        return nullptr;
    }

    /**
     * 에셋의 Shared Pointer를 반환합니다.
     * @return 유효한 에셋 Shared Pointer 또는 nullptr
     */
    [[nodiscard]] std::shared_ptr<T> GetShared() const
    {
        if (slot)
        {
            const std::shared_ptr<IAsset> asset = slot->GetAsset();
            return std::static_pointer_cast<T>(asset);
        }
        return nullptr;
    }

    /** 현재 핸들이 유효한 에셋을 가리키고 있는지 확인합니다. */
    [[nodiscard]] bool IsValid() const
    {
        return slot && slot->GetState() == ELoadingState::Loaded;
    }

    /**
     * 이 핸들이 가리키는 에셋의 고유 ID를 반환합니다.
     * 핸들이 무효하거나 슬롯이 없으면 AssetId::Invalid를 반환합니다.
     */
    [[nodiscard]] AssetId GetAssetId() const
    {
        return slot ? slot->GetAssetId() : AssetId::Invalid;
    }

public:
    [[nodiscard]] T* operator->() const { return Get(); }
    [[nodiscard]] T& operator*() const { return *Get(); }

    [[nodiscard]] explicit operator bool() const { return IsValid(); }
    [[nodiscard]] bool operator==(const AssetHandle& other) const = default;

private:
    std::shared_ptr<AssetSlot> slot;
};
}  // namespace se::asset
