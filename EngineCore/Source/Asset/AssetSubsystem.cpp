#include "Asset/AssetSubsystem.h"

#include "Asset/Loaders/Texture2DLoader.h"
#include "Asset/Types/Texture2D.h"

using namespace se::asset;


bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");
    asset_manager = std::make_unique<AssetManager>();

    asset_manager->RegisterLoader<Texture2D, Texture2DLoader>(".png");
    asset_manager->RegisterLoader<Texture2D, Texture2DLoader>(".jpg");
    asset_manager->RegisterLoader<Texture2D, Texture2DLoader>(".jpeg");

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");
    asset_manager.reset();
}
