#pragma once
#include <memory>

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetSlot.h"
#include "SimpleEngine/Core/Container/HashMap.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
/**
 * 로드된 Asset의 Slot(MemoryBlock)을 관리하는 런타임 저장소
 * Slot의 생명주기(생성 및 제거)를 관리합니다.
 */
class SE_CORE_API AssetCache
{
public:
    AssetCache() = default;
    ~AssetCache() = default;

    // 복사 & 이동 금지
    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;
    AssetCache(AssetCache&&) = delete;
    AssetCache& operator=(AssetCache&&) = delete;

public:
    /**
     * ID에 해당하는 Slot을 찾습니다.
     * @return 존재하면 Slot의 shared_ptr, 없으면 nullptr
     */
    [[nodiscard]] std::shared_ptr<AssetSlot> Find(const AssetId& id) const;

    /**
     * ID에 해당하는 Slot을 찾거나, 없으면 새로 생성하여 등록합니다. (Thread-Safe)
     * @param id 에셋 ID
     * @param type_id 에셋 타입 (새로 생성될 경우 사용)
     * @param asset_path 에셋 경로 (새로 생성될 경우 사용)
     */
    [[nodiscard]] std::shared_ptr<AssetSlot> FindOrCreate(const AssetId& id, const TypeId& type_id, const AssetPath& asset_path);

    /**
     * 특정 Slot을 캐시에서 강제로 제거합니다. (Unload)
     * @warning 해당 Slot을 가리키는 Handle들이 있다면 Dangling 상태가 될 수 있습니다.
     */
    void Remove(const AssetId& id);

    /**
     * 참조 카운트가 1인(캐시만 들고 있고, 외부 Handle은 없는) Slot들을 정리합니다.
     * @return 제거된 Slot의 개수
     */
    uint32 CollectGarbage();

    /** 현재 캐시된 에셋의 총 개수를 반환합니다. */
    [[nodiscard]] uint32 GetCount() const;

private:
    mutable TracySharedLockable(std::shared_mutex, slot_mutex);
    HashMap<AssetId, std::shared_ptr<AssetSlot>> slots;
};
}  // namespace se::asset
