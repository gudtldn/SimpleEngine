#include "SimpleEngine/Asset/HandleTable.h"
#include "SimpleEngine/Utility/Debug.h"

#include <ranges>
#include <utility>


namespace se::asset
{
HandleTable::~HandleTable()
{
    // 모든 Occupied 슬롯의 에셋 데이터를 해제
    for (SlotEntry& slot : slots)
    {
        if (slot.slot_state == SlotEntry::ESlotState::Occupied)
        {
            DestroyAssetData(slot);
        }
    }
}

HandleData HandleTable::FindOrCreate(const AssetId& id, const TypeId& type, const AssetPath& path)
{
    ZoneScopedN("HandleTable::FindOrCreate");

    // Fast path: shared_lock으로 기존 슬롯 검색
    {
        std::shared_lock read_lock(pool_mutex);
        if (const auto found = guid_index.Find(id))
        {
            const uint32 idx = found.Value();
            return HandleData{ idx, slots[idx].generation };
        }
    }

    // Slow path: unique_lock -> double-check -> 슬롯 할당
    std::unique_lock write_lock(pool_mutex);

    // Double-check: 다른 스레드가 shared->unique 승격 사이에 이미 생성했을 수 있음
    if (const auto found = guid_index.Find(id))
    {
        const uint32 idx = found.Value();
        return HandleData{ idx, slots[idx].generation };
    }

    // 슬롯 할당: free_list에서 재사용 또는 신규 생성
    uint32 index;

    if (auto reused = free_list.Pop())
    {
        index = reused.Value();
        slots[index].Initialize(id, type, path);
    }
    else
    {
        index = static_cast<uint32>(slots.Len());
        slots.Emplace(id, type, path);
    }

    guid_index.Insert(id, index);
    return HandleData{ index, slots[index].generation };
}

Optional<HandleData> HandleTable::Find(const AssetId& id) const
{
    std::shared_lock read_lock(pool_mutex);

    if (const auto found = guid_index.Find(id))
    {
        const uint32 idx = found.Value();
        return HandleData{ idx, slots[idx].generation };
    }
    return NullOpt;
}

SlotEntry& HandleTable::GetSlot(uint32 index)
{
    SE_ASSERT(std::cmp_less(index, slots.Len()), "HandleTable::GetSlot - index {} out of range (size: {})", index, slots.Len());
    return slots[index];
}

const SlotEntry& HandleTable::GetSlot(uint32 index) const
{
    SE_ASSERT(std::cmp_less(index, slots.Len()), "HandleTable::GetSlot - index {} out of range (size: {})", index, slots.Len());
    return slots[index];
}

bool HandleTable::IsHandleValid(const HandleData& handle) const
{
    if (
        !handle.IsValid()
        || std::cmp_greater_equal(handle.index, slots.Len()) // handle.index >= slots.Len()
    )
    {
        return false;
    }

    const SlotEntry& entry = slots[handle.index];
    return entry.generation == handle.generation
        && entry.slot_state == SlotEntry::ESlotState::Occupied;
}

void HandleTable::MarkForEviction(uint32 index)
{
    SE_ASSERT(std::cmp_less(index, slots.Len()), "HandleTable::MarkForEviction - index out of range");

    const SlotEntry& entry = slots[index];
    SE_ASSERT(entry.slot_state == SlotEntry::ESlotState::Occupied, "HandleTable::MarkForEviction - slot is not Occupied");
    SE_ASSERT(entry.strong_count.load(std::memory_order_relaxed) == 0, "HandleTable::MarkForEviction - strong_count is not 0");
}

void HandleTable::EvictSlot(uint32 index)
{
    ZoneScopedN("HandleTable::EvictSlot");

    std::unique_lock write_lock(pool_mutex);

    SE_ASSERT(std::cmp_less(index, slots.Len()), "HandleTable::EvictSlot - index out of range");
    SlotEntry& entry = slots[index];
    SE_ASSERT(entry.slot_state == SlotEntry::ESlotState::Occupied, "HandleTable::EvictSlot - slot is not Occupied");
    SE_ASSERT(entry.strong_count.load(std::memory_order_relaxed) == 0, "HandleTable::EvictSlot - strong_count != 0");

    // Slot 해제
    EvictSlotInternal(index, entry);
}

uint32 HandleTable::CollectGarbage()
{
    ZoneScopedN("HandleTable::CollectGarbage");

    std::unique_lock write_lock(pool_mutex);

    uint32 evict_count = 0;
    for (const auto [idx, entry] : slots | std::views::enumerate)
    {
        if (
            entry.slot_state == SlotEntry::ESlotState::Occupied
            && entry.strong_count.load(std::memory_order_relaxed) == 0
        )
        {
            EvictSlotInternal(idx, entry);
            ++evict_count;
        }
    }

    return evict_count;
}

uint32 HandleTable::GetCount() const
{
    std::shared_lock read_lock(pool_mutex);
    return static_cast<uint32>(guid_index.Len());
}

uint32 HandleTable::GetCapacity() const
{
    std::shared_lock read_lock(pool_mutex);
    return static_cast<uint32>(slots.Len());
}

void HandleTable::EvictSlotInternal(uint32 index, SlotEntry& entry)
{
    // 에셋 데이터 해제
    DestroyAssetData(entry);

    // guid_index에서 제거
    guid_index.Remove(entry.asset_id);

    // 슬롯을 Free 상태로 전환 + 내부적으로 세대 번호 증가
    entry.Clear();

    // free_list에 반환 (LIFO)
    free_list.Push(index);
}

void HandleTable::DestroyAssetData(SlotEntry& entry)
{
    AssetBase* ptr = entry.asset.exchange(nullptr, std::memory_order_acq_rel);
    if (!ptr)
    {
        return;
    }

    SE_ASSERT(
        entry.destructor != nullptr,
        "HandleTable::DestroyAssetData - Asset has no destructor! "
        "Did you forget to register this asset type in the TypeRegistry? (Type: {})",
        entry.asset_type.GetName()
    );

    entry.destructor(ptr);
    entry.state.store(ELoadingState::Unloaded, std::memory_order_release);
}
} // namespace se::asset
