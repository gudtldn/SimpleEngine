#pragma once

#include "SimpleEngine/Asset/AssetCache.h"
#include "SimpleEngine/Asset/Pipeline/AssetImporter.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"


namespace se::asset
{
/**
 * @todo docs
 */
class SE_CORE_API AssetSubsystem : public ISubsystem
{
public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem

private:
    std::unique_ptr<AssetImporter> asset_importer;
    std::unique_ptr<AssetCache> asset_cache;
};
}  // namespace se::asset
