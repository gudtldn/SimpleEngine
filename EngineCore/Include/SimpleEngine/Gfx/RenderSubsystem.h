#pragma once
#include <memory>

#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Interfaces/ISubsystem.h"
#include "SimpleEngine/Reflection/SubsystemRegistration.h"
#include "SimpleEngine/Rendering/Manager/PSOManager.h"
#include "SimpleEngine/Rendering/RenderGraph/RenderGraph.h"

#include "SDL3/SDL.h"


class SE_CORE_API RenderSubsystem : public se::core::ISubsystem<PlatformSubsystem>
{
    // TODO: GameServer는 이거 필요없는데
    SE_REGISTER_SUBSYSTEM(RenderSubsystem)

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
    [[nodiscard]] se::rendering::PSOManager* GetPSOManager() const { return pso_manager.get(); }
    [[nodiscard]] se::rendering::RenderGraph* GetRenderGraph() const { return render_graph.get(); }

public:
    [[nodiscard]] SDL_GPUSwapchainComposition DetermineBestSwapchainComposition(SDL_Window* window, const WindowDesc& desc) const;
    [[nodiscard]] SDL_GPUPresentMode DetermineBestPresentMode(SDL_Window* window) const;

private:
    SDL_GPUDevice* gpu_device = nullptr;

    std::unique_ptr<se::rendering::RenderGraph> render_graph;
    std::unique_ptr<se::rendering::PSOManager> pso_manager;
};
