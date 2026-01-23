#pragma once

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"


namespace se::editor::asset
{
class EditorAssetSubsystem : public core::ISubsystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

private:
    void RefreshRegistry();
    void ImportAsset(const std::filesystem::path& physical_path);

    Optional<se::asset::AssetEntry_DEPRECATED> ProcessMetaFile(const std::filesystem::path& physical_path);

private:
    se::asset::AssetManager_DEPRECATED* asset_manager = nullptr;
};
}
