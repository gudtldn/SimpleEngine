export module SE.Editor.Rendering:RenderPass.ImGuiPass;

import SE.Types;
import SE.Rendering;
import SE.Interface.IRenderPass;
import std;


export namespace se::editor::rendering::passes
{
class EditorUIPass : public IRenderPass
{
public:
    virtual void Setup(se::rendering::render_graph::RenderGraphBuilder& builder) override;
    virtual void Execute(const se::rendering::render_graph::RGExecutionContext& context) override;

private:
    se::rendering::render_graph::RGResourceHandle back_buffer_handle;
};
}
