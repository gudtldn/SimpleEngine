#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"


namespace se::editor
{
/**
 * @todo docs
 */
class SE_ANNOTATION(=meta::Internal) EditorUIPass : public se::graphics::RenderPassBase
{
    SE_CLASS(EditorUIPass, se::graphics::RenderPassBase)

public:
    virtual void Setup(se::graphics::RenderGraphBuilder& builder) override;
    virtual void Execute(se::graphics::RGExecutionContext& context) override;
};
}
