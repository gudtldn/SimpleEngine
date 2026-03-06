#pragma once

#include "SimpleEngine/Asset/SlotEntry.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/FunctionRef.h"

#include "tracy/Tracy.hpp"

#include <atomic>
#include <limits>
#include <shared_mutex>


namespace se::asset
{
/**
 * HandleTable에서 반환하는 핸들 데이터 (ECS의 Entity({id, generation})와 동일한 패턴)
 */
struct HandleData
{
    static constexpr uint32 INVALID_INDEX = std::numeric_limits<uint32>::max();

    uint32 index = INVALID_INDEX;
    uint32 generation = 0;

    /**
     * 유효한 핸들인지 확인합니다.
     * @note 발급 여부만 검사하며, 세대 일치 여부는 별도로 확인해야 합니다.
     */
    [[nodiscard]] FORCE_INLINE bool IsValid() const noexcept { return index != INVALID_INDEX; }

    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] bool operator==(const HandleData&) const noexcept = default;
};

/**
 * Generational Handle Table - AssetPool의 내부 저장소를 대체하는 핵심 자료구조
 *
 * @see SlotEntry 개별 슬롯 레이아웃
 * @see HandleData 외부에 반환되는 핸들 값
 */
class SE_CORE_API HandleTable
{
public:
    HandleTable() = default;
    ~HandleTable();

    // 복사 & 이동 금지
    HandleTable(const HandleTable&) = delete;
    HandleTable& operator=(const HandleTable&) = delete;
    HandleTable(HandleTable&&) = delete;
    HandleTable& operator=(HandleTable&&) = delete;

public:
    /** AssetId를 사용하여 기존 슬롯을 찾거나, 존재하지 않으면 새로 생성합니다. (Thread-Safe) */
    [[nodiscard]] HandleData FindOrCreate(const AssetId& id, const TypeId& type, const AssetPath& path);

    /** AssetId를 사용하여 기존 슬롯을 찾습니다. 없으면 잘못된(Invalid) HandleData를 반환합니다. (Thread-Safe) */
    [[nodiscard]] Optional<HandleData> Find(const AssetId& id) const;

    /**
     * 인덱스를 통해 SlotEntry에 직접 접근합니다.
     * @pre index < slots.Len()
     */
    [[nodiscard]] SlotEntry& GetSlot(uint32 index);
    [[nodiscard]] const SlotEntry& GetSlot(uint32 index) const;

    /**
     * HandleData의 generation이 SlotEntry의 generation과 일치하는지 검증합니다.
     * 불일치할 경우 이미 해제되었거나 재사용된(stale) 핸들로 간주합니다.
     */
    [[nodiscard]] bool IsHandleValid(const HandleData& handle) const;

public:
    /**
     * 슬롯을 eviction(해제) 후보로 표시합니다.
     * 현재는 strong_count가 0인지만 검증합니다.
     */
    void MarkForEviction(uint32 index);

    /**
     * 슬롯을 즉시 해제합니다. (Thread-Safe)
     * @pre slots[index].strong_count == 0
     * @pre slots[index].slot_state == Occupied
     */
    void EvictSlot(uint32 index);

    /**
     * strong_count가 0인 모든 Occupied 슬롯을 해제합니다. (Thread-Safe)
     * @return 해제된 슬롯의 수
     */
    uint32 CollectGarbage();

    /** 현재 사용 중인(Occupied) 슬롯의 개수를 반환합니다. */
    [[nodiscard]] uint32 GetCount() const;

    /** 전체 슬롯 배열의 크기(사용 중인 슬롯 + 유휴 슬롯)를 반환합니다. */
    [[nodiscard]] uint32 GetCapacity() const;

    /**
     * 조건에 부합하는 슬롯을 일괄 해제합니다. (Thread-Safe)
     * @param filter strong_count == 0인 Occupied 슬롯 중, true를 반환하는 슬롯만 해제합니다.
     * @param max_count 최대 해제 개수
     * @return 해제된 슬롯의 수
     */
    uint32 EvictWhere(
        FunctionRef<bool(uint32, const SlotEntry&)> filter,
        uint32 max_count = std::numeric_limits<uint32>::max()
    );

    /** 현재 추적 중인 총 에셋 메모리 사용량을 반환합니다. */
    [[nodiscard]] FORCE_INLINE uint64 GetTotalMemoryUsage() const noexcept
    {
        return total_memory.load(std::memory_order_relaxed);
    }

    /** 에셋 메모리 사용량을 추가합니다. (에셋 로딩 완료 시 호출) */
    void TrackMemoryUsage(uint64 bytes);

    /** 에셋 메모리 사용량을 감소합니다. */
    void UntrackMemoryUsage(uint64 bytes);

private:
    /** 락이 이미 획득된 상태에서 슬롯을 실제로 해제하는 내부 헬퍼 함수입니다. */
    void EvictSlotInternal(uint32 index, SlotEntry& entry);

    /** 에셋 포인터를 등록된 소멸자(destructor)를 사용하여 안전하게 해제하는 내부 헬퍼 함수입니다. */
    static void DestroyAssetData(SlotEntry& entry);

private:
    mutable TracySharedLockable(std::shared_mutex, pool_mutex);

    // 실제 데이터가 저장되는 연속된 메모리 풀
    Array<SlotEntry> slots;

    // 재사용 가능한 슬롯 인덱스 목록
    Array<uint32> free_list;

    // AssetId <-> SlotIndex 매핑
    HashMap<AssetId, uint32> guid_index;

    // 총 에셋 메모리 사용량 (Eviction 예산 계산용)
    std::atomic<uint64> total_memory{0};
};
} // namespace se::asset
