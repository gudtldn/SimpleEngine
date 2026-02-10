#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "SDL3/SDL_mouse.h"


namespace se
{
/**
 * 마우스 버튼을 나타내는 열거형
 */
enum class MouseButton : uint8
{
    Left   = SDL_BUTTON_LEFT,    // 1
    Middle = SDL_BUTTON_MIDDLE,  // 2
    Right  = SDL_BUTTON_RIGHT,   // 3
    X1     = SDL_BUTTON_X1,      // 4
    X2     = SDL_BUTTON_X2,      // 5

    Max  = 6,
};

/** SDL 마우스 버튼 값을 MouseButton으로 변환합니다. */
[[nodiscard]] constexpr MouseButton ToMouseButton(uint8 sdl_button) noexcept
{
    return static_cast<MouseButton>(sdl_button);
}
}  // namespace se
