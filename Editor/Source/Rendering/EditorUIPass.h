#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Rendering/RenderPass/IRenderPass.h"


namespace se::editor::rendering
{
class EditorUIPass : public se::rendering::IRenderPass
{
public:
    virtual void Setup(se::rendering::RenderGraphBuilder& builder) override;
    virtual void Execute(se::rendering::RGExecutionContext& context) override;

private:
};
}
