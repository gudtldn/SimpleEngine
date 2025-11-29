#include "Asset/AssetSubsystem.h"

#include "Asset/ImportSettings/TextureImportSettings.h"
#include "Asset/Loaders/Texture2DLoader.h"
#include "Asset/Types/Texture2D.h"
#include "Core/Subsystem/SubsystemRegistration.h"



namespace se::asset
{
SE_REGISTER_SUBSYSTEM(AssetSubsystem);

bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");
    asset_manager = std::make_unique<AssetManager>();

    asset_manager->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".png");
    asset_manager->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpg");
    asset_manager->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpeg");

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");
    asset_manager.reset();
}
}
