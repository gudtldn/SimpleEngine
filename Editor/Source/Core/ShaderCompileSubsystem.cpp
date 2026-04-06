#include "Core/ShaderCompileSubsystem.h"

#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"

#if SE_HAS_HLSL_COMPILER
#include "SDL3_shadercross/SDL_shadercross.h"
#endif


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(ShaderCompileSubsystem)
    .DependsOn<EventSubsystem>();

SE_BEGIN_REFLECT(ShaderCompileSubsystem, meta::Internal)
SE_END_REFLECT(ShaderCompileSubsystem)

bool ShaderCompileSubsystem::Initialize()
{
#if SE_HAS_HLSL_COMPILER
    // SDL_ShaderCross_Init는 내부적으로 DXC 라이브러리를 Load 함
    if (!SDL_ShaderCross_Init())
    {
        ConsoleLog(ELogLevel::Error, "Failed to initialize SDL_ShaderCross!");
        return false;
    }
#endif
    return true;
}

void ShaderCompileSubsystem::Release()
{
#if SE_HAS_HLSL_COMPILER
    SDL_ShaderCross_Quit();
#endif
}
} // namespace se::editor
