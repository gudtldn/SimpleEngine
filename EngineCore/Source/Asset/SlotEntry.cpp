#include "SimpleEngine/Asset/SlotEntry.h"


namespace se::asset
{
SlotEntry::SlotEntry(const AssetId& in_id, const TypeId& in_type, AssetPath in_path)
    : asset_type{ in_type }
    , source_path{ std::move(in_path) }
    , asset_id{ in_id }
    , slot_state{ ESlotState::Occupied }
{
}

SlotEntry::SlotEntry(SlotEntry&& other) noexcept
{
    *this = std::move(other);
}

SlotEntry& SlotEntry::operator=(SlotEntry&& other) noexcept
{
    if (this != &other)
    {
        asset.store(other.asset.exchange(nullptr, std::memory_order_relaxed), std::memory_order_relaxed);
        strong_count.store(other.strong_count.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        weak_count.store(other.weak_count.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        state.store(other.state.exchange(ELoadingState::Unloaded, std::memory_order_relaxed), std::memory_order_relaxed);

        generation = other.generation; // generation은 이동 시에도 유지하는 것이 안전함
        asset_type = std::move(other.asset_type);
        source_path = std::move(other.source_path);
        asset_id = std::move(other.asset_id);

        destructor = std::exchange(other.destructor, nullptr);
        last_access_frame = std::exchange(other.last_access_frame, 0);
        asset_size_bytes = std::exchange(other.asset_size_bytes, 0);
        scope = other.scope;
        slot_state = std::exchange(other.slot_state, ESlotState::Free);
    }
    return *this;
}

void SlotEntry::Initialize(const AssetId& in_id, const TypeId& in_type, AssetPath in_path)
{
    // generation은 기존 값 사용 (Clear에서 이미 증가됨)
    const uint32 preserved_generation = generation;

    *this = SlotEntry{ in_id, in_type, std::move(in_path) };
    generation = preserved_generation;
}

void SlotEntry::Clear()
{
    const uint32 next_generation = generation + 1;
    SE_ASSERT(next_generation < std::numeric_limits<uint32>::max(), "Generation overflow!");

    *this = SlotEntry{};
    generation = next_generation;
}

AssetBase* SlotEntry::ExchangeAsset(AssetBase* new_asset)
{
    return asset.exchange(new_asset, std::memory_order_acq_rel);
}

bool SlotEntry::BeginLoad()
{
    // Unloaded -> Loading
    ELoadingState expected = ELoadingState::Unloaded;
    if (state.compare_exchange_strong(expected, ELoadingState::Loading, std::memory_order_acq_rel))
    {
        return true;
    }

    // Failed -> Loading (재시도)
    expected = ELoadingState::Failed;
    return state.compare_exchange_strong(expected, ELoadingState::Loading, std::memory_order_acq_rel);
}

void SlotEntry::WaitForLoadComplete() const
{
    while (state.load(std::memory_order_acquire) == ELoadingState::Loading)
    {
        state.wait(ELoadingState::Loading, std::memory_order_acquire);
    }
}
} // namespace se::asset
