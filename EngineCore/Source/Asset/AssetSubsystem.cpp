#include "Asset/AssetSubsystem.h"

#include "Asset/ImportSettings_DEPRECATED/TextureImportSettings.h"
#include "Asset/Loaders_DEPRECATED/ObjLoader.h"
#include "Asset/Loaders_DEPRECATED/Texture2DLoader.h"
#include "Asset/Types/MeshTypes.h"
#include "Asset/Types/Texture2D.h"
#include "Core/Subsystem/SubsystemRegistration.h"



namespace se::asset
{
SE_REGISTER_SUBSYSTEM(AssetSubsystem);

bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");
    asset_manager = std::make_unique<AssetManager_DEPRECATED>();

    asset_manager->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".png");
    asset_manager->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpg");
    asset_manager->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpeg");
    asset_manager->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpeg");
    asset_manager->RegisterLoader<StaticMesh, ObjLoader>(".obj");

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");
    asset_manager.reset();
}
}  // namespace se::asset
