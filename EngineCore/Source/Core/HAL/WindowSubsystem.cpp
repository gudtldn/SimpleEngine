#include "SimpleEngine/Core/HAL/WindowSubsystem.h"

#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include <ranges>


namespace se
{
SE_REGISTER_SUBSYSTEM(WindowSubsystem)
    .DependsOn<EventSubsystem>();

SE_BEGIN_REFLECT(WindowSubsystem, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(WindowSubsystem)


bool WindowSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Window Subsystem...");

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        ConsoleLog(ELogLevel::Error, "SDL_InitSubSystem(VIDEO) failed: {}", SDL_GetError());
        return false;
    }
    ConsoleLog(ELogLevel::Info, "SDL_InitSubSystem(VIDEO) succeeded");

    // EventSubsystem의 raw SDL 이벤트를 구독
    EventSubsystem& event_subsystem = GetSubsystemChecked<EventSubsystem>();
    sdl_event_handle = event_subsystem.on_sdl_event.AddLambda([this](const SDL_Event& event)
    {
        OnSDLEvent(event);
    });

    // PrepareWindow()로 메인 윈도우 설정이 준비되었으면 생성
    if (prepared_main_window_desc.HasValue())
    {
        ConsoleLog(ELogLevel::Info, "Creating main window...");

        if (const auto window_result = CreateWindow(*prepared_main_window_desc))
        {
            main_window_id = *window_result;
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "{}", window_result.Error().What());
            return false;
        }

        SDL_ShowWindow(GetWindow(main_window_id));
        ConsoleLog(ELogLevel::Info, "Main window created");
    }

    ConsoleLog(ELogLevel::Info, "Window Subsystem initialized");
    return true;
}

void WindowSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Window Subsystem...");

    // 이벤트 구독 해제
    if (sdl_event_handle.IsValid())
    {
        if (EventSubsystem* event_subsystem = GetSubsystem<EventSubsystem>())
        {
            event_subsystem->on_sdl_event.Remove(sdl_event_handle);
        }
        sdl_event_handle.Invalidate();
    }

    for (const auto& [native_handle, _] : windows | std::views::values)
    {
        SDL_DestroyWindow(native_handle);
    }
    windows.Clear();
    main_window_id = 0;
    focused_window_id = 0;

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void WindowSubsystem::PrepareWindow(WindowDesc&& window_desc)
{
    prepared_main_window_desc = std::move(window_desc);
}

Expected<SDL_WindowID, WindowCreateError> WindowSubsystem::CreateWindow(const WindowDesc& window_desc)
{
    const char* window_title_c = window_desc.title.CStr();
    SDL_Window* new_window = SDL_CreateWindow(
        window_title_c,
        static_cast<int>(window_desc.width),
        static_cast<int>(window_desc.height),
        window_desc.sdl_window_flags
    );

    if (!new_window)
    {
        return Unexpected<WindowCreateError>{
            WindowCreateError::CreationFailed,
            String::Format("SDL_CreateWindow failed: {}", SDL_GetError())
        };
    }

    const SDL_WindowID new_window_id = SDL_GetWindowID(new_window);
    RegisterWindow(new_window_id, new_window, window_desc);

    // GPU Claim은 RenderSubsystem에서 등록된 Delegate에서 수행
    on_window_created.Broadcast(new_window_id, new_window, window_desc);

    return new_window_id;
}

bool WindowSubsystem::DestroyWindow(SDL_WindowID window_id)
{
    if (!windows.Contains(window_id) || window_id == main_window_id)
    {
        return false;
    }

    SDL_Window* window = windows.FindChecked(window_id).native_handle;

    // 파괴 직전 Broadcast -> RenderSubsystem이 GPU Release 수행
    on_window_destroyed.Broadcast(window_id, window);

    SDL_DestroyWindow(window);
    UnregisterWindow(window_id);
    return true;
}

SDL_Window* WindowSubsystem::GetWindow(SDL_WindowID window_id) const
{
    return windows.Find(window_id).Map([](const WindowEntry& entry)
    {
        return entry.native_handle;
    })
    .ValueOrDefault();
}

Optional<const WindowDesc&> WindowSubsystem::GetWindowDesc(SDL_WindowID window_id) const
{
    return windows.Find(window_id).Map([](const WindowEntry& entry) -> const WindowDesc&
    {
        return entry.desc;
    });
}

Optional<const WindowEntry&> WindowSubsystem::GetWindowEntry(SDL_WindowID window_id) const
{
    return windows.Find(window_id);
}

bool WindowSubsystem::HasWindow(SDL_WindowID window_id) const
{
    return windows.Contains(window_id);
}

u32 WindowSubsystem::GetWindowCount() const
{
    return static_cast<u32>(windows.Len());
}

bool WindowSubsystem::IsFullscreen(SDL_WindowID window_id) const
{
    if (SDL_Window* window = GetWindow(window_id))
    {
        return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
    }
    return false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void WindowSubsystem::SetFullscreen(SDL_WindowID window_id, bool fullscreen) // NOLINT(*-make-member-function-const)
{
    if (SDL_Window* window = GetWindow(window_id))
    {
        if (IsFullscreen(window_id) != fullscreen)
        {
            SDL_SetWindowFullscreen(window, fullscreen);
        }
    }
}

void WindowSubsystem::RegisterWindow(SDL_WindowID window_id, SDL_Window* window, const WindowDesc& desc)
{
    windows.Emplace(window_id, WindowEntry{
        .native_handle = window,
        .desc = desc,
    });
}

void WindowSubsystem::UnregisterWindow(SDL_WindowID window_id)
{
    windows.Remove(window_id);
}

void WindowSubsystem::OnSDLEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        focused_window_id = event.window.windowID;
        on_window_focus_gained.Broadcast(event.window.windowID);
        break;

    case SDL_EVENT_WINDOW_FOCUS_LOST:
        on_window_focus_lost.Broadcast(event.window.windowID);
        if (focused_window_id == event.window.windowID)
        {
            focused_window_id = 0;
        }
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        on_window_resized.Broadcast(
            event.window.windowID,
            static_cast<u32>(event.window.data1),
            static_cast<u32>(event.window.data2)
        );
        break;

    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        on_window_close_requested.Broadcast(event.window.windowID);
        break;

    default:
        break;
    }
}
} // namespace se
