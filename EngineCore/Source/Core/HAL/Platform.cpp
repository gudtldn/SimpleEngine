#include "Core/HAL/Platform.h"

#include "SDL3/SDL.h"


namespace se::platform
{
Path GetExecutableDirectory()
{
    return { SDL_GetBasePath() };
}
}  // namespace se::platform
