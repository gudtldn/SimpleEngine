#include "Core/HAL/Platform.h"

#include "SDL3/SDL.h"


namespace se
{
Path Platform::GetExecutableDirectory()
{
    return { SDL_GetBasePath() };
}
}  // namespace se
