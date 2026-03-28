#pragma once

#include "SimpleEngine/Core/Functional/FunctionRef.h"
#include "SimpleEngine/Core/Functional/MultiDelegate.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Graphics/Manager/PSOManager.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphExecutor.h"

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
     * GPU Resource를 업로드 후, 각 Window별로 RenderGraph를 구축하여 프레임을 렌더링합니다.
     * @param upload_fn 데이터 전송(Copy)을 위한 콜백 (void(SDL_GPUCommandBuffer*))
     * @param build_fn Window별 Pass 구성을 위한 콜백 (void(RGTextureHandle swapchain, RenderGraphBuilder& builder))
     */
    void RenderFrame(
        FunctionRef<void(SDL_GPUCommandBuffer*)> upload_fn,
        FunctionRef<void(graphics::RGTextureHandle, graphics::RenderGraphBuilder&)> build_fn
    ) const;

public:
    [[nodiscard]] graphics::RenderDevice& GetRenderDevice() const { return *render_device; }
    [[nodiscard]] graphics::PSOManager& GetPSOManager() const { return *pso_manager; }
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
    std::unique_ptr<graphics::RenderGraphBuilder> render_graph_builder;
    std::unique_ptr<graphics::RenderGraphExecutor> render_graph_executor;
    std::unique_ptr<graphics::PSOManager> pso_manager;
    std::unique_ptr<graphics::GpuResourceManager> resource_manager;

    DelegateHandle window_created_handle;
    DelegateHandle window_destroyed_handle;
};
} // namespace se
