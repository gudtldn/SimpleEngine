#include "Asset/AssetSlot.h"


namespace se::asset
{
AssetSlot::AssetSlot(AssetId id, Path path, const TypeId& type_id)
    : asset_id(id)
    , asset_type(type_id)
    , source_path(std::move(path))
    , loading_state(ELoadingState::Unloaded)
{
}

std::shared_ptr<IAsset> AssetSlot::GetAsset() const
{
    std::shared_lock lock(mutex);
    return asset;
}

void AssetSlot::SetAsset(std::shared_ptr<IAsset> new_asset, ELoadingState new_state)
{
    std::unique_lock lock(mutex);
    asset = std::move(new_asset);
    SetState(new_state);
}

ELoadingState AssetSlot::GetState() const
{
    return loading_state.load(std::memory_order_acquire);
}

void AssetSlot::SetState(ELoadingState state)
{
    loading_state.store(state, std::memory_order_release);
}

void AssetSlot::Invalidate()
{
    std::unique_lock lock(mutex);
    asset.reset();
    SetState(ELoadingState::Unloaded);
}
} // namespace se::asset
