#pragma once
#include "SimpleEngine/Rendering/RenderPass/IRenderPass.h"
#include "SimpleEngine/Rendering/RenderGraph/RGResourceHandle.h"


namespace se::editor::rendering
{
class EditorUIPass : public se::rendering::IRenderPass
{
public:
    virtual void Setup(se::rendering::RenderGraphBuilder& builder) override;
    virtual void Execute(se::rendering::RGExecutionContext& context) override;

private:
    se::rendering::RGResourceHandle back_buffer_handle;
};
}
