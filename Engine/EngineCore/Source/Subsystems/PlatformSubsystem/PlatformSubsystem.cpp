module SE.Subsystems.PlatformSubsystem;

import SE.Core;
import SE.Utility;
import SE.Subsystems.Utility;
import SE.Subsystems.RenderSubsystem;
import <SDL3/SDL_gpu.h>;

using namespace se::utility::string_utils;


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
        if (const auto window_result = CreateWindow(*main_window_info))
        {
            main_window_id = *window_result;
        }
        else
        {
            ConsoleLog(ELogLevel::Error, window_result.error().message);
        }

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

std::expected<SDL_WindowID, WindowCreateError> PlatformSubsystem::CreateWindow(const WindowDesc& window_desc)
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
        return std::unexpected{
            WindowCreateError::WindowCreation(
                ToU8String(std::format("SDL_CreateWindow failed: {}", SDL_GetError()))
            )
        };
    }

    const SDL_WindowID new_window_id = SDL_GetWindowID(new_window);
    RegisterWindow(new_window_id, new_window);

    if (const RenderSubsystem* render_subsystem = GetSubsystemUnchecked<const RenderSubsystem>())
    {
        if (SDL_GPUDevice* device = render_subsystem->GetGpuDevice())
        {
            if (!SDL_ClaimWindowForGPUDevice(device, new_window))
            {
                SDL_DestroyWindow(new_window);
                return std::unexpected{
                    WindowCreateError::GPUDeviceClaim(
                        ToU8String(std::format("SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError()))
                    )
                };
            }

            const SDL_GPUSwapchainComposition composition =
                window_desc.swapchain_composition.ValueOr(render_subsystem->DetermineBestSwapchainComposition(new_window, window_desc));
            const SDL_GPUPresentMode present_mode = window_desc.present_mode.ValueOr(render_subsystem->DetermineBestPresentMode(new_window));

            if (!SDL_SetGPUSwapchainParameters(device, new_window, composition, present_mode))
            {
                SDL_DestroyWindow(new_window);
                return std::unexpected{
                    WindowCreateError::SwapchainSetup(
                        ToU8String(std::format("SDL_SetGPUSwapchainParameters failed: {}", SDL_GetError()))
                    )
                };
            }
        }
    }

    return new_window_id;
}

bool PlatformSubsystem::DestroyWindow(SDL_WindowID window_id)
{
    if (!windows.contains(window_id) || window_id == main_window_id)
    {
        return false;
    }

    if (const RenderSubsystem* render_subsystem = GetSubsystemUnchecked<const RenderSubsystem>())
    {
        if (SDL_GPUDevice* device = render_subsystem->GetGpuDevice())
        {
            SDL_ReleaseWindowFromGPUDevice(device, windows.at(window_id));
        }
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
