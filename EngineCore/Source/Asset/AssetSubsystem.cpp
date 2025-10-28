#include "Asset/AssetSubsystem.h"

using namespace se::asset;


bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");
    asset_manager = std::make_unique<AssetManager>();

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");
    asset_manager.reset();
}
