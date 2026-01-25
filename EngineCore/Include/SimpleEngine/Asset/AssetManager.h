#pragma once
#include <atomic>
#include <memory>

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/IAsset.h"
#include "SimpleEngine/Core/Concurrency/Coroutine.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::asset
{
/**
 * @todo docs
 */
enum class ELoadingState : uint8
{
    Unloaded,
    Loading,
    Loaded,
    Failed,
};

/**
 * @todo docs
 */
struct AssetEntry
{
    refl::TypeId asset_type;
    std::weak_ptr<IAsset> asset;

    std::atomic<ELoadingState> loading_state = ELoadingState::Unloaded;
};

/**
 * @todo docs
 */
class SE_CORE_API AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

public:
    template <typename AssetType>
    [[nodiscard]] std::shared_ptr<AssetType> LoadAsset_Test(const VPath& asset_path);

    // 추후 사용할 API
    // template <typename AssetType>
    // [[nodiscard]] concurrency::Task<std::shared_ptr<AssetType>> LoadAsync(...);

    // template <typename AssetType>
    // [[nodiscard]] std::shared_ptr<AssetType> LoadSynchronous(const Guid& guid) const;

private:
    HashMap<AssetId, AssetEntry> assets;
};

template <typename AssetType>
std::shared_ptr<AssetType> AssetManager::LoadAsset_Test([[maybe_unused]] const VPath& asset_path)
{
    return nullptr;
}
}  // namespace se::asset
