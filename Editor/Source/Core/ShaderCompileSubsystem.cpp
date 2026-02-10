#include "Core/ShaderCompileSubsystem.h"

#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"

#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(ShaderCompileSubsystem)
    .DependsOn<PlatformSubsystem>();

SE_BEGIN_REFLECT(ShaderCompileSubsystem)
SE_END_REFLECT(ShaderCompileSubsystem)

bool ShaderCompileSubsystem::Initialize()
{
    if (!SDL_ShaderCross_Init())
    {
        ConsoleLog(ELogLevel::Error, "Failed to initialize SDL_ShaderCross!");
        return false;
    }
    return true;
}

void ShaderCompileSubsystem::Release()
{
    SDL_ShaderCross_Quit();
}
}
