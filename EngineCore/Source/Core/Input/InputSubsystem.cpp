// ReSharper disable CppMemberFunctionMayBeStatic
#include "SimpleEngine/Core/Input/InputSubsystem.h"

#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se
{
SE_REGISTER_SUBSYSTEM(InputSubsystem)
    .DependsOn<PlatformSubsystem>();

SE_BEGIN_REFLECT(InputSubsystem, meta::Internal)
SE_END_REFLECT(InputSubsystem)


bool InputSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Input Subsystem...");

    if (!SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
    {
        ConsoleLog(ELogLevel::Error, "SDL_InitSubSystem failed: {}", SDL_GetError());
        return false;
    }

    // PlatformSubsystem의 SDL 이벤트를 구독
    PlatformSubsystem& platform = GetSubsystemChecked<PlatformSubsystem>();
    sdl_event_handle = platform.on_sdl_event.AddLambda([this](const SDL_Event& event)
    {
        ProcessSDLEvent(event);
    });

    ConsoleLog(ELogLevel::Info, "Input Subsystem initialized");
    return true;
}

void InputSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Input Subsystem...");

    if (sdl_event_handle.IsValid())
    {
        PlatformSubsystem& platform = GetSubsystemChecked<PlatformSubsystem>();
        platform.on_sdl_event.Remove(sdl_event_handle);
        sdl_event_handle.Invalidate();
    }

    SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMEPAD);
}

void InputSubsystem::BeginFrame()
{
    // 현재 상태를 이전 상태로 복사
    previous_keys = current_keys;
    previous_mouse_buttons = current_mouse_buttons;

    // 마우스 델타/휠은 매 프레임 리셋
    mouse_delta_x = 0.0f;
    mouse_delta_y = 0.0f;
    mouse_wheel_x = 0.0f;
    mouse_wheel_y = 0.0f;
}

// ── Keyboard Event ──────────────────────────────────

bool InputSubsystem::IsKeyDown(EKeyCode key) const
{
    const auto index = static_cast<uint16>(key);
    return index < KEY_COUNT && current_keys[index];
}

bool InputSubsystem::IsKeyPressed(EKeyCode key) const
{
    const auto index = static_cast<uint16>(key);
    return index < KEY_COUNT && current_keys[index] && !previous_keys[index];
}

bool InputSubsystem::IsKeyReleased(EKeyCode key) const
{
    const auto index = static_cast<uint16>(key);
    return index < KEY_COUNT && !current_keys[index] && previous_keys[index];
}

// ── Mouse Event ─────────────────────────────────────

bool InputSubsystem::IsMouseButtonDown(EMouseButton button) const
{
    const auto index = static_cast<uint8>(button);
    return index < MOUSE_BUTTON_COUNT && current_mouse_buttons[index];
}

bool InputSubsystem::IsMouseButtonPressed(EMouseButton button) const
{
    const auto index = static_cast<uint8>(button);
    return index < MOUSE_BUTTON_COUNT && current_mouse_buttons[index] && !previous_mouse_buttons[index];
}

bool InputSubsystem::IsMouseButtonReleased(EMouseButton button) const
{
    const auto index = static_cast<uint8>(button);
    return index < MOUSE_BUTTON_COUNT && !current_mouse_buttons[index] && previous_mouse_buttons[index];
}

// ── Cursor Management ───────────────────────────────

void InputSubsystem::SetCursorVisible(bool visible)
{
    if (visible)
    {
        SDL_ShowCursor();
    }
    else
    {
        SDL_HideCursor();
    }
}

bool InputSubsystem::IsCursorVisible() const
{
    return SDL_CursorVisible();
}

void InputSubsystem::SetRelativeMouseMode(bool enabled)
{
    // TODO: 다중 윈도우 지원하게되면 여기 수정해야함
    const PlatformSubsystem& platform = GetSubsystemChecked<PlatformSubsystem>();
    SDL_SetWindowRelativeMouseMode(platform.GetMainWindow(), enabled);
}

bool InputSubsystem::IsRelativeMouseMode() const
{
    // TODO: 다중 윈도우 지원하게되면 여기 수정해야함
    const PlatformSubsystem& platform = GetSubsystemChecked<const PlatformSubsystem>();
    return SDL_GetWindowRelativeMouseMode(platform.GetMainWindow());
}

// ── SDL Event Processing ────────────────────────────

void InputSubsystem::ProcessSDLEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
    {
        if (!event.key.repeat)
        {
            const uint16 scancode = static_cast<uint16>(event.key.scancode);
            if (scancode < KEY_COUNT)
            {
                current_keys[scancode] = true;
            }
        }
        break;
    }
    case SDL_EVENT_KEY_UP:
    {
        const uint16 scancode = static_cast<uint16>(event.key.scancode);
        if (scancode < KEY_COUNT)
        {
            current_keys[scancode] = false;
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        const uint8 button = event.button.button;
        if (button < MOUSE_BUTTON_COUNT)
        {
            current_mouse_buttons[button] = true;
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        const uint8 button = event.button.button;
        if (button < MOUSE_BUTTON_COUNT)
        {
            current_mouse_buttons[button] = false;
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
    {
        mouse_x = event.motion.x;
        mouse_y = event.motion.y;
        mouse_delta_x += event.motion.xrel;
        mouse_delta_y += event.motion.yrel;
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
    {
        mouse_wheel_x += event.wheel.x;
        mouse_wheel_y += event.wheel.y;
        break;
    }
    default:
        break;
    }
}
} // namespace se
