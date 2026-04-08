#pragma once

#include "SimpleEditor/EditorAPI.h"

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
class GizmoDrawList;

/** 기즈모를 렌더링하는 패스 */
class SE_EDITOR_API GizmoPass : public se::graphics::RenderPassBase
{
    SE_CLASS(GizmoPass, se::graphics::RenderPassBase)

public:
    GizmoPass(
        const GizmoDrawList& in_draw_list,
        const graphics::RenderView& in_render_view,
        graphics::RGTextureHandle in_color_target,
        graphics::RGTextureHandle in_depth_target
    );

    virtual void Setup(graphics::RGSetupContext& context) override;
    virtual void Execute(graphics::RGExecutionContext& context) override;

private:
    const GizmoDrawList& draw_list;
    graphics::RenderView render_view;
    graphics::RGTextureHandle color_target_handle;
    graphics::RGTextureHandle depth_target_handle;
};
} // namespace se::editor
