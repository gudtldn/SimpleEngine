module SimpleEngine.Subsystems.PlatformSubsystem;

import SimpleEngine.Core;
import <SDL3/SDL_gpu.h>;


PlatformSubsystem::PlatformSubsystem(uint32 in_sdl_init_flags)
    : sdl_init_flags(in_sdl_init_flags)
{
}

bool PlatformSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing Platform Subsystem...");
    if (!SDL_Init(sdl_init_flags))
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_Init failed: {}", SDL_GetError());
        return false;
    }
    ConsoleLog(ELogLevel::Info, u8"SDL_Init succeeded");

    if (main_window_info.HasValue())
    {
        ConsoleLog(ELogLevel::Info, u8"Initializing Window...");

        // const float main_display_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        main_window_id = CreateWindow(*main_window_info);

        SDL_ShowWindow(GetWindow(main_window_id));
        ConsoleLog(ELogLevel::Info, u8"Window initialized");
    }
    return true;
}

void PlatformSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, u8"Releasing Platform Subsystem...");

    for (SDL_Window* window : windows | std::views::values)
    {
        SDL_DestroyWindow(window);
    }
    windows.clear();
    main_window_id = 0;

    SDL_Quit();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void PlatformSubsystem::PollEvents()
{
    PlatformEventDispatcher& dispatcher = GetEventDispatcher();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        PlatformEvent platform_event = { .sdl_event = event };
        dispatcher.Dispatch(platform_event);
    }
}

void PlatformSubsystem::PrepareWindow(WindowDesc&& window_desc)
{
    main_window_info = std::move(window_desc);
}

SDL_WindowID PlatformSubsystem::CreateWindow(const WindowDesc& window_desc)
{
    const char* window_title_c = reinterpret_cast<const char*>(window_desc.title.c_str());
    SDL_Window* new_window = SDL_CreateWindow(
        window_title_c,
        window_desc.width,
        window_desc.height,
        window_desc.sdl_window_flags
    );

    if (!new_window)
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_CreateWindow failed: {}", SDL_GetError());
        return 0;
    }

    const SDL_WindowID new_window_id = SDL_GetWindowID(new_window);
    RegisterWindow(new_window_id, new_window);
    return new_window_id;
}

bool PlatformSubsystem::DestroyWindow(SDL_WindowID window_id)
{
    if (!windows.contains(window_id) || window_id == main_window_id)
    {
        return false;
    }

    SDL_DestroyWindow(windows.at(window_id));
    UnregisterWindow(window_id);
    return true;
}

SDL_Window* PlatformSubsystem::GetWindow(SDL_WindowID window_id) const
{
    if (windows.contains(window_id))
    {
        return windows.at(window_id);
    }
    return nullptr;
}

void PlatformSubsystem::RegisterWindow(SDL_WindowID window_id, SDL_Window* window)
{
    windows[window_id] = window;
}

void PlatformSubsystem::UnregisterWindow(SDL_WindowID window_id)
{
    windows.erase(window_id);
}
