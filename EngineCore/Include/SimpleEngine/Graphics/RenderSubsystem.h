#pragma once
#include <memory>

#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"
#include "SimpleEngine/Graphics/Manager/PSOManager.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraph.h"

#include "SDL3/SDL.h"


namespace se
{
class SE_CORE_API RenderSubsystem : public ISubsystem
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
    [[nodiscard]] graphics::PSOManager& GetPSOManager() const { return *pso_manager; }
    [[nodiscard]] graphics::RenderGraph& GetRenderGraph() const { return *render_graph; }
    [[nodiscard]] graphics::GpuResourceManager& GetResourceManager() const { return *resource_manager; }

public:
    [[nodiscard]] SDL_GPUSwapchainComposition DetermineBestSwapchainComposition(SDL_Window* window, const WindowDesc& desc) const;
    [[nodiscard]] SDL_GPUPresentMode DetermineBestPresentMode(SDL_Window* window) const;

private:
    SDL_GPUDevice* gpu_device = nullptr;

    std::unique_ptr<graphics::RenderGraph> render_graph;
    std::unique_ptr<graphics::PSOManager> pso_manager;
    std::unique_ptr<graphics::GpuResourceManager> resource_manager;
};
}  // namespace se
