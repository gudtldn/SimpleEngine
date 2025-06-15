export module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Interfaces.ISubsystem;
import SimpleEngine.Interfaces.IRenderable;
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
     * Subsystem을 렌더링 목록에 추가합니다.
     * @param renderable_system 추가할 Subsystem
     */
    void RegisterRenderableSystem(IRenderable* renderable_system);

    /**
     * Subsystem을 렌더링 목록에서 제거합니다.
     * @param renderable_system 제거할 Subsystem
     */
    void UnregisterRenderableSystem(IRenderable* renderable_system);

    /**
     * 등록된 Subsystem을 렌더링 합니다.
     */
    void RenderFrame() const;

public:
    [[nodiscard]] SDL_GPUDevice* GetGpuDevice() const { return gpu_device; }

private:
    SDL_GPUDevice* gpu_device = nullptr;
    SDL_Window* cached_window = nullptr;

    SDL_GPUSwapchainComposition swapchain_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;

    // Render가 필요한 Subsystem 목록
    std::vector<IRenderable*> renderable_systems;
};
