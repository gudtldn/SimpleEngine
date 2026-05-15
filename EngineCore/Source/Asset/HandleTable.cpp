#include "SimpleEngine/Asset/HandleTable.h"
#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Utility/Debug.h"

#include <ranges>
#include <utility>


namespace se
{
HandleTable::HandleTable()
{
    // TODO: 나중에 ChunkedArray 같은 컨테이너로 개선
    // 멀티스레드 환경에서 Reallocation시 Dangling Ref 위험이 있기에 임시로 Reserve로 메모리 예약
    slots.Reserve(DEFAULT_SLOT_CAPACITY);
}

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
            const u32 idx = found.Value();
            return HandleData{ idx, slots[idx].generation };
        }
    }

    // Slow path: unique_lock -> double-check -> 슬롯 할당
    std::unique_lock write_lock(pool_mutex);

    // Double-check: 다른 스레드가 shared->unique 승격 사이에 이미 생성했을 수 있음
    if (const auto found = guid_index.Find(id))
    {
        const u32 idx = found.Value();
        return HandleData{ idx, slots[idx].generation };
    }

    // 슬롯 할당: free_list에서 재사용 또는 신규 생성
    u32 index;

    if (auto reused = free_list.Pop())
    {
        index = reused.Value();
        slots[index].Initialize(id, type, path);
    }
    else
    {
        if (!SE_ENSURE(
            slots.Len() < slots.Capacity(),
            "HandleTable: slot capacity exhausted! (capacity: {})", slots.Capacity()
        ))
        {
            return {};
        }

        index = static_cast<u32>(slots.Len());
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
        const u32 idx = found.Value();
        return HandleData{ idx, slots[idx].generation };
    }
    return NullOpt;
}

SlotEntry& HandleTable::GetSlot(u32 index)
{
    SE_ASSERT(std::cmp_less(index, slots.Len()), "HandleTable::GetSlot - index {} out of range (size: {})", index, slots.Len());
    return slots[index];
}

const SlotEntry& HandleTable::GetSlot(u32 index) const
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

void HandleTable::MarkForEviction(u32 index)
{
    SE_ASSERT(std::cmp_less(index, slots.Len()), "HandleTable::MarkForEviction - index out of range");
    SlotEntry& entry = slots[index];

    SE_ASSERT(entry.slot_state == SlotEntry::ESlotState::Occupied, "HandleTable::MarkForEviction - slot is not Occupied");
    SE_ASSERT(entry.ref_count.load(std::memory_order_relaxed) == 0, "HandleTable::MarkForEviction - ref_count is not 0");

    // LRU 판단을 위해 "마지막으로 사용된 프레임"을 기록
    entry.last_access_frame.store(Engine::GetFrameCount(), std::memory_order_relaxed);
}

void HandleTable::EvictSlot(u32 index, Array<AssetPayload>& out_deferred)
{
    ZoneScopedN("HandleTable::EvictSlot");

    std::unique_lock write_lock(pool_mutex);

    SE_ASSERT(std::cmp_less(index, slots.Len()), "HandleTable::EvictSlot - index out of range");
    SlotEntry& entry = slots[index];

    SE_ASSERT(entry.slot_state == SlotEntry::ESlotState::Occupied, "HandleTable::EvictSlot - slot is not Occupied");
    SE_ASSERT(entry.ref_count.load(std::memory_order_relaxed) == 0, "HandleTable::EvictSlot - ref_count != 0");

    // Slot 해제
    EvictSlotInternal(index, entry, out_deferred);
}

u32 HandleTable::CollectGarbage(Array<AssetPayload>& out_deferred)
{
    ZoneScopedN("HandleTable::CollectGarbage");

    std::unique_lock write_lock(pool_mutex);

    u32 evict_count = 0;
    for (const auto [idx, entry] : slots | std::views::enumerate)
    {
        if (
            entry.slot_state == SlotEntry::ESlotState::Occupied                     // 슬롯이 사용중이고,
            && entry.ref_count.load(std::memory_order_relaxed) == 0                 // 참조 카운트가 0이며,
            && entry.state.load(std::memory_order_acquire) == ELoadingState::Loaded // 에셋이 완전히 로딩된 경우
        )
        {
            EvictSlotInternal(static_cast<u32>(idx), entry, out_deferred);
            ++evict_count;
        }
    }

    return evict_count;
}

u32 HandleTable::GetCount() const
{
    std::shared_lock read_lock(pool_mutex);
    return static_cast<u32>(guid_index.Len());
}

u32 HandleTable::GetCapacity() const
{
    std::shared_lock read_lock(pool_mutex);
    return static_cast<u32>(slots.Capacity());
}

u32 HandleTable::EvictWhere(
    FunctionRef<bool(u32, const SlotEntry&)> filter,
    Array<AssetPayload>& out_deferred,
    u32 max_count
)
{
    ZoneScopedN("HandleTable::EvictWhere");

    std::unique_lock write_lock(pool_mutex);

    u32 count = 0;
    for (auto [idx, entry] : slots | std::views::enumerate)
    {
        const u32 slot_index = static_cast<u32>(idx);

        // max_count를 넘어간 경우
        if (count >= max_count)
        {
            break;
        }

        // 아직 할당되지 않은 슬롯의 경우 (빈 슬롯)
        if (entry.slot_state != SlotEntry::ESlotState::Occupied)
        {
            continue;
        }

        // 아직 참조하고 있는 Handle이 있는 경우
        if (entry.ref_count.load(std::memory_order_relaxed) != 0)
        {
            continue;
        }

        // Asset이 로딩중인 경우
        if (entry.state.load(std::memory_order_acquire) == ELoadingState::Loading)
        {
            continue;
        }

        // Filter에 부합하지 않는경우
        if (!filter(slot_index, entry))
        {
            continue;
        }

        EvictSlotInternal(slot_index, entry, out_deferred);
        ++count;
    }

    return count;
}

void HandleTable::EvictSlotInternal(u32 index, SlotEntry& entry, Array<AssetPayload>& out_deferred)
{
    // 메모리 사용량 차감
    UntrackMemoryUsage(entry.asset_size_bytes);

    // 에셋 데이터를 지연 파괴 목록으로 이동 (Frame-Epoch 보장)
    if (AssetBase* ptr = entry.asset.exchange(nullptr, std::memory_order_acq_rel))
    {
        SE_ASSERT(
            entry.destructor != nullptr,
            "HandleTable::EvictSlotInternal - Asset has no destructor! (Type: {})", entry.asset_type.GetName()
        );
        out_deferred.Push(AssetPayload{ ptr, entry.destructor });
    }

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
        "Did you forget to register this asset type in the TypeRegistry? (Type: {})", entry.asset_type.GetName()
    );

    entry.destructor(ptr);
    entry.state.store(ELoadingState::Unloaded, std::memory_order_release);
}
} // namespace se
