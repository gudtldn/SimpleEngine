export module SimpleEngine.Platform.SdlSubsystem;

import SimpleEngine.Platform.Types;
import std;
import <SDL3/SDL.h>;


export class SdlSubsystem
{
public:
    SdlSubsystem() = default;
    ~SdlSubsystem() = default;

    SdlSubsystem(const SdlSubsystem&) = delete;
    SdlSubsystem& operator=(const SdlSubsystem&) = delete;
    SdlSubsystem(SdlSubsystem&&) = delete;
    SdlSubsystem& operator=(SdlSubsystem&&) = delete;

public:
    [[nodiscard]] bool Initialize(uint32 sdl_init_flags);
    void Release();

    void PollEvents(std::vector<SDL_Event>& out_events);

    [[nodiscard]] bool CreateWindowAndGpuDevice(
        const std::u8string& window_title, uint32 window_width, uint32 window_height, uint32 sdl_window_flags
    );

public:
    SDL_Window* GetWindow() const { return window; }
    SDL_GPUDevice* GetGpuDevice() const { return gpu_device; }

private:
    SDL_Window* window = nullptr;
    SDL_GPUDevice* gpu_device = nullptr;
};
