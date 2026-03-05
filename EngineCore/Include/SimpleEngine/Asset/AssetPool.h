#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/HandleTable.h"


namespace se::asset
{
/**
 * 로드된 Asset의 Slot(MemoryBlock)을 관리하는 런타임 저장소
 * SlotEntry의 생명주기(생성 및 제거)를 관리합니다.
 */
class SE_CORE_API AssetPool
{
public:
    AssetPool() = default;
    ~AssetPool() = default;

    // 복사 & 이동 금지
    AssetPool(const AssetPool&) = delete;
    AssetPool& operator=(const AssetPool&) = delete;
    AssetPool(AssetPool&&) = delete;
    AssetPool& operator=(AssetPool&&) = delete;

public:
    /** AssetId로 HandleData를 조회합니다. */
    [[nodiscard]] Optional<HandleData> Find(const AssetId& id) const;

    /** AssetId로 HandleData를 조회하거나 없으면 새로 생성합니다. (Thread-Safe) */
    [[nodiscard]] HandleData FindOrCreate(const AssetId& id, const TypeId& type_id, const AssetPath& asset_path);

    /**
     * 특정 슬롯을 캐시에서 강제로 제거합니다. (Unload)
     * @warning 해당 슬롯을 가리키는 Handle들이 있다면 stale 상태가 될 수 있습니다.
     */
    void Remove(const AssetId& id);

    /**
     * strong_count가 0인(외부 Handle이 없는) 슬롯들을 정리합니다.
     * @return 제거된 슬롯의 개수
     */
    uint32 CollectGarbage();

    /** 현재 캐시된 에셋의 총 개수를 반환합니다. */
    [[nodiscard]] uint32 GetCount() const;

public:
    /** HandleTable에 직접 접근합니다. */
    [[nodiscard]] FORCE_INLINE HandleTable& GetTable() { return table; }
    [[nodiscard]] FORCE_INLINE const HandleTable& GetTable() const { return table; }

private:
    HandleTable table;
};
} // namespace se::asset
