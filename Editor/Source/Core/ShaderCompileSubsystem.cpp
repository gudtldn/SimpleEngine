#include "Core/ShaderCompileSubsystem.h"

#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"

#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(ShaderCompileSubsystem)
    .DependsOn<EventSubsystem>();

SE_BEGIN_REFLECT(ShaderCompileSubsystem, meta::Internal)
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
