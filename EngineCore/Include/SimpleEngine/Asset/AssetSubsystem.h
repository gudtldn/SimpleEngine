#pragma once
#include "SimpleEngine/Asset/AssetManager.h"
#include "SimpleEngine/Core/Interfaces/ISubsystem.h"
#include "SimpleEngine/Reflection/SubsystemRegistration.h"


/**
 *
 */
class SE_CORE_API AssetSubsystem : public se::core::ISubsystem<>
{
    SE_REGISTER_SUBSYSTEM(AssetSubsystem)

public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem

public:
    [[nodiscard]] se::asset::AssetManager& GetAssetManager() const { return *asset_manager; }

private:
    std::unique_ptr<se::asset::AssetManager> asset_manager;
};
