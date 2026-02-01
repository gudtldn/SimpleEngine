#include "Core/HAL/PlatformSubsystem.h"

#include <ranges>

#include "Core/Logging/Logging.h"
#include "Core/Subsystem/SubsystemRegistration.h"
#include "Gfx/RenderSubsystem.h"
#include "Utility/SubsystemUtils.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_gpu.h"

using namespace se::event;
using namespace se;

namespace se
{
SE_REGISTER_SUBSYSTEM(PlatformSubsystem);

PlatformSubsystem::PlatformSubsystem(uint32 in_sdl_init_flags)
    : sdl_init_flags(in_sdl_init_flags)
{
    using namespace se;

    // SDL_SetMemoryFunctions(
    //     [](usize size) static -> void*
    //     {
    //         return OsMemory::Allocate(size);
    //     },
    //     [](usize nmemb, usize size) static -> void*
    //     {
    //         const usize total_size = nmemb * size;
    //         void* mem = OsMemory::Allocate(total_size);
    //         std::memset(mem, 0, total_size);
    //          return mem;
    //     },
    //     [](void* mem, usize size) static -> void*
    //     {
    //         return OsMemory::Realloc(mem, size, alignof(max_align_t));
    //     },
    //     OsMemory::Free
    // );
}

bool PlatformSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Platform Subsystem...");
    if (!SDL_Init(sdl_init_flags))
    {
        ConsoleLog(ELogLevel::Error, "SDL_Init failed: {}", SDL_GetError());
        return false;
    }
    ConsoleLog(ELogLevel::Info, "SDL_Init succeeded");

    if (main_window_info.HasValue())
    {
        ConsoleLog(ELogLevel::Info, "Initializing Window...");

        if (const auto window_result = CreateWindow(*main_window_info))
        {
            main_window_id = *window_result;
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "{}", window_result.Error().What());
            return false;
        }

        SDL_ShowWindow(GetWindow(main_window_id));
        ConsoleLog(ELogLevel::Info, "Window initialized");
    }
    return true;
}

void PlatformSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Platform Subsystem...");

    for (SDL_Window* window : windows | std::views::values)
    {
        SDL_DestroyWindow(window);
    }
    windows.Clear();
    main_window_id = 0;

    SDL_Quit();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void PlatformSubsystem::PollEvents()
{
    EventDispatcher& dispatcher = GetEventDispatcher();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        PlatformEvent platform_event = { .sdl_event = event };
        dispatcher.Dispatch(platform_event);
    }
}

bool PlatformSubsystem::IsFullscreen() const
{
    return (SDL_GetWindowFlags(GetMainWindow()) & SDL_WINDOW_FULLSCREEN) != 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void PlatformSubsystem::SetFullscreen(bool fullscreen) // NOLINT(*-make-member-function-const)
{
    if (IsFullscreen() == fullscreen)
    {
        return;
    }
    SDL_SetWindowFullscreen(GetMainWindow(), fullscreen);
}

void PlatformSubsystem::PrepareWindow(WindowDesc&& window_desc)
{
    main_window_info = std::move(window_desc);
}

Expected<SDL_WindowID, WindowCreateError> PlatformSubsystem::CreateWindow(const WindowDesc& window_desc)
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
        return Unexpected{
            WindowCreateError::WindowCreation(
                String::Format("SDL_CreateWindow failed: {}", SDL_GetError())
            )
        };
    }

    const SDL_WindowID new_window_id = SDL_GetWindowID(new_window);
    RegisterWindow(new_window_id, new_window);

    if (const RenderSubsystem* render_subsystem = se::GetSubsystem<const RenderSubsystem>())
    {
        if (SDL_GPUDevice* device = render_subsystem->GetGpuDevice())
        {
            if (!SDL_ClaimWindowForGPUDevice(device, new_window))
            {
                SDL_DestroyWindow(new_window);
                return Unexpected{
                    WindowCreateError::GPUDeviceClaim(
                        String::Format("SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError())
                    )
                };
            }

            const SDL_GPUSwapchainComposition composition =
                window_desc.swapchain_composition.ValueOr(render_subsystem->DetermineBestSwapchainComposition(new_window, window_desc));
            const SDL_GPUPresentMode present_mode = window_desc.present_mode.ValueOr(render_subsystem->DetermineBestPresentMode(new_window));

            if (!SDL_SetGPUSwapchainParameters(device, new_window, composition, present_mode))
            {
                SDL_DestroyWindow(new_window);
                return Unexpected{
                    WindowCreateError::SwapchainSetup(
                        String::Format("SDL_SetGPUSwapchainParameters failed: {}", SDL_GetError())
                    )
                };
            }
        }
    }

    return new_window_id;
}

bool PlatformSubsystem::DestroyWindow(SDL_WindowID window_id)
{
    if (!windows.Contains(window_id) || window_id == main_window_id)
    {
        return false;
    }

    if (const RenderSubsystem* render_subsystem = se::GetSubsystem<const RenderSubsystem>())
    {
        if (SDL_GPUDevice* device = render_subsystem->GetGpuDevice())
        {
            SDL_ReleaseWindowFromGPUDevice(device, *windows.Find(window_id));
        }
    }

    SDL_DestroyWindow(*windows.Find(window_id));
    UnregisterWindow(window_id);
    return true;
}

SDL_Window* PlatformSubsystem::GetWindow(SDL_WindowID window_id) const
{
    if (Optional window_opt = windows.Find(window_id))
    {
        return *window_opt;
    }
    return nullptr;
}

void PlatformSubsystem::RegisterWindow(SDL_WindowID window_id, SDL_Window* window)
{
    windows.Emplace(window_id, window);
}

void PlatformSubsystem::UnregisterWindow(SDL_WindowID window_id)
{
    windows.Remove(window_id);
}
}  // namespace se
