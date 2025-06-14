module SimpleEngine.Subsystems.PlatformSubsystem;

import SimpleEngine.Logging;
import <SDL3/SDL_gpu.h>;


PlatformSubsystem::PlatformSubsystem(uint32 in_sdl_init_flags)
    : sdl_init_flags(in_sdl_init_flags)
{
}

void PlatformSubsystem::PrepareWindow(const std::u8string& window_title, uint32 window_width, uint32 window_height, uint32 sdl_window_flags)
{
    window_info = {
        .title = window_title,
        .width = window_width,
        .height = window_height,
        .sdl_window_flags = sdl_window_flags,
    };
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

    // TODO: 나중에 다중모니터 지원하도록 변경
    ConsoleLog(ELogLevel::Info, u8"Initializing Window...");
    if (!window_info.has_value())
    {
        ConsoleLog(ELogLevel::Error, u8"Window info is not set! Please call PrepareWindow before InitializeEngine");
        return false;
    }

    const float main_display_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    const char* window_title_c = reinterpret_cast<const char*>(window_info->title.c_str());
    window = SDL_CreateWindow(
        window_title_c,
        static_cast<int>(static_cast<float>(window_info->width) * main_display_scale),
        static_cast<int>(static_cast<float>(window_info->height) * main_display_scale),
        window_info->sdl_window_flags
    );

    if (!window)
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }
    SDL_ShowWindow(window);
    ConsoleLog(ELogLevel::Info, u8"Window initialized");

    return true;
}

void PlatformSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, u8"Releasing Platform Subsystem...");
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void PlatformSubsystem::PollEvents(std::vector<SDL_Event>& out_events)
{
    // TODO: Event Dispatcher 만들기
    out_events.clear();
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        out_events.push_back(event);
    }
}
