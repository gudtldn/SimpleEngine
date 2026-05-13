#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
// forward declaration
enum class EViewMode : u8;

/**
 * @todo docs
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) WorldGridPass : public se::RenderPassBase
{
    SE_CLASS(WorldGridPass, se::RenderPassBase)

public:
    explicit WorldGridPass(
        EViewMode in_view_mode,
        const RenderView& in_render_view,
        RGTextureHandle in_color_target_handle,
        RGTextureHandle in_depth_target_handle
    );

    virtual void Setup(RGSetupContext& context) override;
    virtual void Execute(RGExecutionContext& context) override;

private:
    const EViewMode view_mode;
    const RenderView render_view;
    RGTextureHandle color_target_handle;
    RGTextureHandle depth_target_handle;
};
} // namespace se::editor
