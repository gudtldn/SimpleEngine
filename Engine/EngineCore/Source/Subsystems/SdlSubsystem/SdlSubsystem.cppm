export module SimpleEngine.Subsystems.SdlSubsystem;

import SimpleEngine.Platform.Types;
import SimpleEngine.Core.ISubSystem;
import std;
import <SDL3/SDL.h>;


export class SdlSubsystem : public ISubSystem
{
public:
    SdlSubsystem() = default;
    virtual ~SdlSubsystem() override = default;

    SdlSubsystem(const SdlSubsystem&) = delete;
    SdlSubsystem& operator=(const SdlSubsystem&) = delete;
    SdlSubsystem(SdlSubsystem&&) = delete;
    SdlSubsystem& operator=(SdlSubsystem&&) = delete;

public:
    /** SDL_Init에 들어갈 Flag를 설정합니다. */
    void SetSdlInitFlags(uint32 in_sdl_init_flags);

    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    void PollEvents(std::vector<SDL_Event>& out_events);

    [[nodiscard]] bool CreateWindowAndGpuDevice(
        const std::u8string& window_title, uint32 window_width, uint32 window_height, uint32 sdl_window_flags
    );

public:
    SDL_Window* GetWindow() const { return window; }
    SDL_GPUDevice* GetGpuDevice() const { return gpu_device; }

private:
    uint32 sdl_init_flags = 0;

    SDL_Window* window = nullptr;
    SDL_GPUDevice* gpu_device = nullptr;
};
