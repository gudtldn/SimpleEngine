export module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Rendering;
import SimpleEngine.Interface.ISubsystem;
import SimpleEngine.Subsystems.PlatformSubsystem;
import std;
import <SDL3/SDL.h>;

using namespace se::rendering::manager;
using namespace se::rendering::render_graph;
using namespace se::rendering::passes;


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
    [[nodiscard]] ShaderManager* GetShaderManager() const { return shader_manager.get(); }
    [[nodiscard]] PSOManager* GetPSOManager() const { return pso_manager.get(); }

public:
    SDL_GPUSwapchainComposition DetermineBestSwapchainComposition(SDL_Window* window, const WindowDesc& desc) const;
    SDL_GPUPresentMode DetermineBestPresentMode(SDL_Window* window) const;

private:
    SDL_GPUDevice* gpu_device = nullptr;

    std::unique_ptr<RenderGraph> render_graph;
    std::unique_ptr<ShaderManager> shader_manager;
    std::unique_ptr<PSOManager> pso_manager;
};
