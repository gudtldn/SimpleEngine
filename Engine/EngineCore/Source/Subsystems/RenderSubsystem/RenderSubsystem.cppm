export module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Interface.ISubsystem;
import SimpleEngine.Subsystems.PlatformSubsystem;
import std;
import <SDL3/SDL.h>;


export class RenderSubsystem : public ISubsystem<PlatformSubsystem>
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    /**
     * 등록된 Subsystem을 렌더링 합니다.
     */
    [[deprecated("Use Begin/EndFrame() instead")]]
    void RenderFrame() const;

    void BeginFrame() const;
    void EndFrame() const;
    void SubmitCommands() const;

public:
    [[nodiscard]] SDL_GPUDevice* GetGpuDevice() const { return gpu_device; }

public:
    SDL_GPUSwapchainComposition DetermineBestSwapchainComposition(SDL_Window* window, const WindowDesc& desc) const;
    SDL_GPUPresentMode DetermineBestPresentMode(SDL_Window* window) const;

private:
    SDL_GPUDevice* gpu_device = nullptr;
    SDL_Window* cached_main_window = nullptr;
};
