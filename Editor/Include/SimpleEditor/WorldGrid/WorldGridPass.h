#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
// forward declaration
enum class EViewMode : uint8;

/**
 * @todo docs
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) WorldGridPass : public se::graphics::RenderPassBase
{
    SE_CLASS(WorldGridPass, se::graphics::RenderPassBase)

public:
    explicit WorldGridPass(
        EViewMode in_view_mode,
        const graphics::RenderView& in_render_view,
        graphics::RGTextureHandle in_color_target_handle,
        graphics::RGTextureHandle in_depth_target_handle
    );

    virtual void Setup(graphics::RGSetupContext& context) override;
    virtual void Execute(graphics::RGExecutionContext& context) override;

private:
    const EViewMode view_mode;
    const graphics::RenderView render_view;
    graphics::RGTextureHandle color_target_handle;
    graphics::RGTextureHandle depth_target_handle;
};
} // namespace se::editor
