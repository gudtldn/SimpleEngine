#pragma once

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"


namespace se::editor
{
/**
 * ImGui UI를 Swapchain에 렌더링하는 패스
 * @note 추후 CompositePass 같은거 만들어서, UIPass에서 하는 역할을 나눠야 할 듯
 */
class SE_ANNOTATION(=meta::Internal) EditorUIPass : public se::graphics::RenderPassBase
{
    SE_CLASS(EditorUIPass, se::graphics::RenderPassBase)

public:
    explicit EditorUIPass(
        se::graphics::RGTextureHandle in_back_buffer,
        Array<se::graphics::RGTextureHandle> in_viewport_colors
    );

    virtual void Setup(se::graphics::RGSetupContext& context) override;
    virtual void Execute(se::graphics::RGExecutionContext& context) override;

private:
    se::graphics::RGTextureHandle swapchain_handle;
    Array<se::graphics::RGTextureHandle> viewport_color_handles;
};
} // namespace se::editor
