#pragma once

#include "SimpleEngine/Asset/AssetManager_DEPRECATED.h"
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
    [[nodiscard]] AssetManager_DEPRECATED& GetAssetManager() const { return *asset_manager; }

private:
    std::unique_ptr<AssetManager_DEPRECATED> asset_manager;
};
}
