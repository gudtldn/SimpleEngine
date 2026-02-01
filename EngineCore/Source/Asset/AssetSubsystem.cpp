#include "Asset/AssetSubsystem.h"

#include "Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "Asset/Pipeline/Translators/AssimpTranslator.h"
#include "Core/Logging/Logging.h"
#include "Core/Subsystem/SubsystemRegistration.h"


namespace se::asset
{
SE_REGISTER_SUBSYSTEM(AssetSubsystem);

bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");
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
}
} // namespace se::asset
