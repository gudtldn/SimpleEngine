#pragma once
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Interfaces/ISubsystem.h"


namespace se::editor::asset
{
class EditorAssetSubsystem : public core::ISubsystem<AssetSubsystem>
{
    SE_REGISTER_SUBSYSTEM(EditorAssetSubsystem)

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

private:
    void RefreshRegistry();
    void ImportAsset(const std::filesystem::path& physical_path);

    Optional<se::asset::AssetEntry> ProcessMetaFile(const std::filesystem::path& physical_path);

private:
    se::asset::AssetManager* asset_manager = nullptr;
};
}
