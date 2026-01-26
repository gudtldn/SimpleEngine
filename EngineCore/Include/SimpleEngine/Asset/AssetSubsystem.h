#pragma once

// DEPRECATED
#include "SimpleEngine/Asset/AssetManager_DEPRECATED.h"
// ~DEPRECATED

#include "SimpleEngine/Asset/AssetCache.h"
#include "SimpleEngine/Asset/Pipeline/AssetImporter.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"


namespace se::asset
{
/**
 * @todo docs
 */
class SE_CORE_API AssetSubsystem : public se::core::ISubsystem
{
public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem

public:
    [[nodiscard]] AssetImporter& GetAssetImporter() const { return *asset_importer; }
    [[nodiscard]] AssetCache& GetAssetCache() const { return *asset_cache; }

public:
    [[nodiscard]] AssetManager_DEPRECATED& GetAssetManager_DEPRECATED() const { return *asset_manager_deprecated; }

private:
    std::unique_ptr<AssetManager_DEPRECATED> asset_manager_deprecated;

    std::unique_ptr<AssetImporter> asset_importer;
    std::unique_ptr<AssetCache> asset_cache;
};
}  // namespace se::asset
