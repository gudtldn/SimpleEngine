#pragma once

#include "SimpleEngine/Core/Functional/MultiDelegate.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Graphics/Manager/PSOManager.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraph.h"

#include "SDL3/SDL.h"

#include <memory>


namespace se
{
class SE_CORE_API SE_ANNOTATION(=meta::Internal) RenderSubsystem : public SubsystemBase
{
    SE_CLASS(RenderSubsystem, SubsystemBase)

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
    [[nodiscard]] graphics::RenderDevice& GetRenderDevice() const { return *render_device; }
    [[nodiscard]] graphics::PSOManager& GetPSOManager() const { return *pso_manager; }
    [[nodiscard]] graphics::RenderGraph& GetRenderGraph() const { return *render_graph; }
    [[nodiscard]] graphics::GpuResourceManager& GetResourceManager() const { return *resource_manager; }

public:
    [[nodiscard]] SDL_GPUSwapchainComposition DetermineBestSwapchainComposition(SDL_Window* window, const WindowDesc& desc) const;
    [[nodiscard]] SDL_GPUPresentMode DetermineBestPresentMode(SDL_Window* window) const;

private:
    /** WindowSubsystem의 on_window_created에서 호출되어 GPU Claim 및 스왑체인 설정을 수행합니다. */
    void OnWindowCreated(SDL_WindowID window_id, SDL_Window* window, const WindowDesc& desc);

    /** WindowSubsystem의 on_window_destroyed에서 호출되어 GPU Release를 수행합니다. */
    void OnWindowDestroyed(SDL_WindowID window_id, SDL_Window* window);

private:
    std::unique_ptr<graphics::RenderDevice> render_device;
    std::unique_ptr<graphics::RenderGraph> render_graph;
    std::unique_ptr<graphics::PSOManager> pso_manager;
    std::unique_ptr<graphics::GpuResourceManager> resource_manager;

    DelegateHandle window_created_handle;
    DelegateHandle window_destroyed_handle;
};
}  // namespace se
