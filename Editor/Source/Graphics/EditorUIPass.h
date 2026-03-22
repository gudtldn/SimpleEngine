#pragma once

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"


namespace se::editor
{
/**
 * ImGui UI를 스왑체인 BackBuffer에 렌더링하는 패스
 */
class SE_ANNOTATION(=meta::Internal) EditorUIPass : public se::graphics::RenderPassBase
{
    SE_CLASS(EditorUIPass, se::graphics::RenderPassBase)

public:
    explicit EditorUIPass(se::graphics::RGTextureHandle in_back_buffer)
        : swapchain_handle(in_back_buffer)
    {
    }

    virtual void Setup(se::graphics::RGSetupContext& context) override;
    virtual void Execute(se::graphics::RGExecutionContext& context) override;

private:
    se::graphics::RGTextureHandle swapchain_handle;
};
} // namespace se::editor
