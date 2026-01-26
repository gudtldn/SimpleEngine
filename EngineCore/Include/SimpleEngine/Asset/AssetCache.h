#pragma once
#include <atomic>
#include <filesystem>
#include <memory>

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/IAsset.h"
#include "SimpleEngine/Core/Container/HashMap.h"
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
class SE_CORE_API AssetCache
{
public:
    AssetCache() = default;
    ~AssetCache() = default;

    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;
    AssetCache(AssetCache&&) = delete;
    AssetCache& operator=(AssetCache&&) = delete;

private:
    /**
     * @todo docs
     */
    struct AssetEntry
    {
        std::weak_ptr<IAsset> asset;

        refl::TypeId asset_type;
        std::filesystem::path file_path;
        std::atomic<ELoadingState> loading_state = ELoadingState::Unloaded;
    };

    HashMap<AssetId, AssetEntry> asset_caches;
};
}  // namespace se::asset
