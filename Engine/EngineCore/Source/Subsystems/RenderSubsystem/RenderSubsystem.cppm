export module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Interfaces.ISubsystem;
import std;
import <SDL3/SDL.h>;


export class RenderSubsystem : public ISubsystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    virtual std::vector<::std::type_index> GetDependencies() const override;

    /**
     * Swapchain 파라미터를 미리 설정합니다.
     * @note 이 함수는 Initialize()보다 먼저 호출되어야 합니다.
     */
    void ConfigureSwapchain(SDL_GPUSwapchainComposition in_composition, SDL_GPUPresentMode in_present_mode);

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

private:
    SDL_GPUDevice* gpu_device = nullptr;
    SDL_Window* cached_window = nullptr;

    SDL_GPUSwapchainComposition swapchain_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
};
