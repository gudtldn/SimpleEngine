#include "Asset/AssetSubsystem.h"

// DEPRECATED
#include "Asset/ImportSettings_DEPRECATED/TextureImportSettings.h"
#include "Asset/Loaders_DEPRECATED/ObjLoader.h"
#include "Asset/Loaders_DEPRECATED/Texture2DLoader.h"
// ~DEPRECATED

#include "Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "Asset/Pipeline/Translators/AssimpTranslator.h"
#include "Asset/Types/MeshTypes.h"
#include "Asset/Types/Texture2D.h"
#include "Core/Subsystem/SubsystemRegistration.h"


namespace se::asset
{
SE_REGISTER_SUBSYSTEM(AssetSubsystem);

bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");
    {
        asset_manager_deprecated = std::make_unique<AssetManager_DEPRECATED>();

        asset_manager_deprecated->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".png");
        asset_manager_deprecated->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpg");
        asset_manager_deprecated->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpeg");
        asset_manager_deprecated->RegisterLoader<Texture2D, Texture2DLoader, TextureImportSettings>(".jpeg");
        asset_manager_deprecated->RegisterLoader<StaticMesh, ObjLoader>(".obj");
    }

    {
        // Make AssetImporter Instance
        asset_importer = std::make_unique<AssetImporter>();

        // Register Translator
        asset_importer->RegisterTranslator<AssimpTranslator>();

        // Register Factory
        asset_importer->RegisterFactory<StaticMeshFactory>();
    }

    {
        // Make AssetCache Instance
        asset_cache = std::make_unique<AssetCache>();
    }

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");

    asset_cache.reset();
    asset_importer.reset();

    {
        asset_manager_deprecated.reset();
    }
}
} // namespace se::asset
