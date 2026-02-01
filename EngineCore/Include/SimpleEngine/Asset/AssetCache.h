#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetSlot.h"
#include "SimpleEngine/Core/Container/HashMap.h"


namespace se::asset
{
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
    HashMap<AssetId, AssetSlot> asset_caches;
};
}  // namespace se::asset
