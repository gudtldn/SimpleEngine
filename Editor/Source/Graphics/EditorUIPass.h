#pragma once

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"


namespace se::editor
{
/**
 * ImGui UI를 Swapchain에 렌더링하는 패스
 * @note 추후 CompositePass 같은거 만들어서, UIPass에서 하는 역할을 나눠야 할 듯
 */
class SE_ANNOTATION(=meta::Internal) EditorUIPass : public se::RenderPassBase
{
    SE_CLASS(EditorUIPass, se::RenderPassBase)

public:
    explicit EditorUIPass(
        se::RGTextureHandle in_back_buffer,
        Array<se::RGTextureHandle> in_viewport_colors
    );

    virtual void Setup(se::RGSetupContext& context) override;
    virtual void Execute(se::RGExecutionContext& context) override;

private:
    se::RGTextureHandle swapchain_handle;
    Array<se::RGTextureHandle> viewport_color_handles;
};
} // namespace se::editor
