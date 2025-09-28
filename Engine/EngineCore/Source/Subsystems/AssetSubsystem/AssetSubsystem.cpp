module;
#include "tracy/Tracy.hpp"
module SE.Subsystems.AssetSubsystem;

import SE.Core;

using namespace se::assets;


bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing Asset subsystem...");
    asset_manager = std::make_unique<AssetManager>();

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, u8"Releasing Asset subsystem...");
    asset_manager.reset();
}
