#include "SimpleEngine/Asset/AssetSlot.h"


namespace se::asset
{
AssetSlot::AssetSlot(const AssetId& id, const TypeId& type_id, AssetPath path)
    : cached_asset(nullptr)
    , asset_id(id)
    , asset_type(type_id)
    , source_path(std::move(path))
    , loading_state(ELoadingState::Unloaded)
{
}

AssetBase* AssetSlot::GetRawAsset() const
{
    return cached_asset.load(std::memory_order_acquire);
}

std::shared_ptr<AssetBase> AssetSlot::GetAsset() const
{
    std::shared_lock lock(mutex);
    return asset;
}

std::shared_ptr<AssetBase> AssetSlot::ExchangeAsset(std::shared_ptr<AssetBase> new_asset, ELoadingState new_state)
{
    std::unique_lock lock(mutex);

    std::shared_ptr<AssetBase> old_asset = std::exchange(asset, std::move(new_asset));
    cached_asset.store(asset.get(), std::memory_order_release);
    SetState(new_state);

    return old_asset;
}

ELoadingState AssetSlot::GetState() const
{
    return loading_state.load(std::memory_order_acquire);
}

void AssetSlot::SetState(ELoadingState state)
{
    loading_state.store(state, std::memory_order_release);
    loading_state.notify_all();
}

bool AssetSlot::BeginLoad()
{
    // Unloaded 상태라면 Loading으로 변경
    ELoadingState expected = ELoadingState::Unloaded;
    if (loading_state.compare_exchange_strong(expected, ELoadingState::Loading, std::memory_order_acq_rel))
    {
        return true;
    }

    // 만약 이전에 실패했던 슬롯(Failed)이라면 다시 로딩 시도
    expected = ELoadingState::Failed;
    return loading_state.compare_exchange_strong(expected, ELoadingState::Loading, std::memory_order_acq_rel);
}

void AssetSlot::WaitForLoadComplete() const
{
    while (loading_state.load(std::memory_order_acquire) == ELoadingState::Loading)
    {
        loading_state.wait(ELoadingState::Loading);
    }
}

std::shared_ptr<AssetBase> AssetSlot::Invalidate()
{
    std::unique_lock lock(mutex);

    std::shared_ptr<AssetBase> old_asset = std::move(asset);
    cached_asset.store(nullptr, std::memory_order_release);
    SetState(ELoadingState::Unloaded);

    return old_asset;
}
} // namespace se::asset
