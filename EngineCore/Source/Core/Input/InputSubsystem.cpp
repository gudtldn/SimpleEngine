// ReSharper disable CppMemberFunctionMayBeStatic
// NOLINTBEGIN(*-convert-member-functions-to-static)

#include "SimpleEngine/Core/Input/InputSubsystem.h"

#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se
{
SE_REGISTER_SUBSYSTEM(InputSubsystem)
    .DependsOn<EventSubsystem, WindowSubsystem>();

SE_BEGIN_REFLECT(InputSubsystem, meta::Internal)
SE_END_REFLECT(InputSubsystem)


bool InputSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Input Subsystem...");

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
    {
        ConsoleLog(ELogLevel::Error, "SDL_InitSubSystem(GAMEPAD) failed: {}", SDL_GetError());
        return false;
    }

    // EventSubsystem의 SDL 이벤트를 구독
    EventSubsystem& event_subsystem = GetSubsystemChecked<EventSubsystem>();
    sdl_event_handle = event_subsystem.on_sdl_event.AddLambda([this](const SDL_Event& event)
    {
        OnSDLEvent(event);
    });

    ConsoleLog(ELogLevel::Info, "Input Subsystem initialized");
    return true;
}

void InputSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Input Subsystem...");

    if (sdl_event_handle.IsValid())
    {
        if (EventSubsystem* event_subsystem = GetSubsystem<EventSubsystem>())
        {
            event_subsystem->on_sdl_event.Remove(sdl_event_handle);
        }
        sdl_event_handle.Invalidate();
    }

    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void InputSubsystem::BeginFrame()
{
    // 현재 상태를 이전 상태로 복사
    previous_keys = current_keys;
    previous_mouse_buttons = current_mouse_buttons;

    // 마우스 델타/휠은 매 프레임 리셋
    mouse_delta = Vector2f::Zero();
    mouse_wheel = Vector2f::Zero();
}

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

bool InputSubsystem::HasModifier(BitFlags<EModifier> modifiers) const
{
    if (modifiers.IsSet(EModifier::Shift) && !IsKeyDown(EKeyCode::LeftShift) && !IsKeyDown(EKeyCode::RightShift)) { return false; }
    if (modifiers.IsSet(EModifier::Ctrl)  && !IsKeyDown(EKeyCode::LeftCtrl)  && !IsKeyDown(EKeyCode::RightCtrl))  { return false; }
    if (modifiers.IsSet(EModifier::Alt)   && !IsKeyDown(EKeyCode::LeftAlt)   && !IsKeyDown(EKeyCode::RightAlt))   { return false; }
    if (modifiers.IsSet(EModifier::Gui)   && !IsKeyDown(EKeyCode::LeftGui)   && !IsKeyDown(EKeyCode::RightGui))   { return false; }
    return true;
}

bool InputSubsystem::HasAnyModifier(BitFlags<EModifier> modifiers) const
{
    if (modifiers.IsSet(EModifier::Shift) && (IsKeyDown(EKeyCode::LeftShift) || IsKeyDown(EKeyCode::RightShift))) { return true; }
    if (modifiers.IsSet(EModifier::Ctrl)  && (IsKeyDown(EKeyCode::LeftCtrl)  || IsKeyDown(EKeyCode::RightCtrl)))  { return true; }
    if (modifiers.IsSet(EModifier::Alt)   && (IsKeyDown(EKeyCode::LeftAlt)   || IsKeyDown(EKeyCode::RightAlt)))   { return true; }
    if (modifiers.IsSet(EModifier::Gui)   && (IsKeyDown(EKeyCode::LeftGui)   || IsKeyDown(EKeyCode::RightGui)))   { return true; }
    return false;
}

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

Vector2f InputSubsystem::GetGlobalMousePosition() const
{
    float x, y; // NOLINT(*-isolate-declaration)
    SDL_GetGlobalMouseState(&x, &y);
    return { x, y };
}

void InputSubsystem::SetGlobalMousePosition(const Vector2f& pos)
{
    SDL_WarpMouseGlobal(pos.x, pos.y);
}

void InputSubsystem::SetLocalMousePosition(const Vector2f& pos)
{
    const WindowSubsystem& window_subsystem = GetSubsystemChecked<const WindowSubsystem>();
    if (const SDL_WindowID focused_id = window_subsystem.GetFocusedWindowID())
    {
        if (SDL_Window* window = window_subsystem.GetWindow(focused_id))
        {
            SDL_WarpMouseInWindow(window, pos.x, pos.y);
        }
    }
    else if (SDL_Window* main_window = window_subsystem.GetMainWindow())
    {
        SDL_WarpMouseInWindow(main_window, pos.x, pos.y);
    }
}

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
    const WindowSubsystem& window_subsystem = GetSubsystemChecked<const WindowSubsystem>();
    if (const SDL_WindowID focused_id = window_subsystem.GetFocusedWindowID())
    {
        SetRelativeMouseMode(focused_id, enabled);
    }
    else if (SDL_Window* main_window = window_subsystem.GetMainWindow())
    {
        SDL_SetWindowRelativeMouseMode(main_window, enabled);
    }
}

void InputSubsystem::SetRelativeMouseMode(SDL_WindowID window_id, bool enabled)
{
    const WindowSubsystem& window_subsystem = GetSubsystemChecked<const WindowSubsystem>();
    if (SDL_Window* window = window_subsystem.GetWindow(window_id))
    {
        SDL_SetWindowRelativeMouseMode(window, enabled);
    }
}

bool InputSubsystem::IsRelativeMouseMode() const
{
    const WindowSubsystem& window_subsystem = GetSubsystemChecked<const WindowSubsystem>();
    if (const SDL_WindowID focused_id = window_subsystem.GetFocusedWindowID())
    {
        return IsRelativeMouseMode(focused_id);
    }
    if (SDL_Window* main_window = window_subsystem.GetMainWindow())
    {
        return SDL_GetWindowRelativeMouseMode(main_window);
    }
    return false;
}

bool InputSubsystem::IsRelativeMouseMode(SDL_WindowID window_id) const
{
    const WindowSubsystem& window_subsystem = GetSubsystemChecked<const WindowSubsystem>();
    if (SDL_Window* window = window_subsystem.GetWindow(window_id))
    {
        return SDL_GetWindowRelativeMouseMode(window);
    }
    return false;
}

void InputSubsystem::OnSDLEvent(const SDL_Event& event)
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
        mouse_position = { event.motion.x, event.motion.y };
        mouse_delta += { event.motion.xrel, event.motion.yrel };
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
    {
        mouse_wheel += { event.wheel.x, event.wheel.y };
        break;
    }
    default:
        break;
    }
}
} // namespace se

// NOLINTEND(*-convert-member-functions-to-static)
