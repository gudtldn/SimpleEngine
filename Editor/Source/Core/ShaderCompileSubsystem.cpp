#include "Core/ShaderCompileSubsystem.h"
#include "SDL3_shadercross/SDL_shadercross.h"


bool ShaderCompileSubsystem::Initialize()
{
    if (!SDL_ShaderCross_Init())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to initialize SDL_ShaderCross!");
        return false;
    }
    return true;
}

void ShaderCompileSubsystem::Release()
{
    SDL_ShaderCross_Quit();
}
