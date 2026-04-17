#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::graphics
{
// forward declaration
struct SceneDrawData;
class GpuResourceManager;

/**
 * SceneDrawData의 오브젝트를 Forward 렌더링하는 패스
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) ForwardScenePass : public RenderPassBase
{
    SE_CLASS(ForwardScenePass, RenderPassBase)

public:
    explicit ForwardScenePass(
        const SceneDrawData& in_draw_data,
        const GpuResourceManager& in_gpu_manager,
        const RenderView& in_render_view,
        RGTextureHandle in_color_target,
        RGTextureHandle in_depth_target
    );

    virtual void Setup(RGSetupContext& context) override;
    virtual void Execute(RGExecutionContext& context) override;

private:
    const SceneDrawData& draw_data;
    const GpuResourceManager& gpu_manager;
    RenderView render_view;

    RGTextureHandle color_target_handle;
    RGTextureHandle depth_target_handle;
};
} // namespace se::graphics
