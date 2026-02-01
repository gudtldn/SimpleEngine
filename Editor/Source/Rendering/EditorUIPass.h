#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Rendering/RenderPass/IRenderPass.h"


namespace se::editor
{
class EditorUIPass : public se::graphics::IRenderPass
{
public:
    virtual void Setup(se::graphics::RenderGraphBuilder& builder) override;
    virtual void Execute(se::graphics::RGExecutionContext& context) override;
};
}
