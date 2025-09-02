export module SE.Rendering:RenderPass.ClearPass;
import :RenderGraph;

import SE.Interface.IRenderPass;
import <SDL3/SDL_gpu.h>;


namespace se::rendering::passes
{
export class [[deprecated]] ClearPass : public IRenderPass
{
public:
    ClearPass(render_graph::RGResourceHandle target, const SDL_FColor& color);

    virtual void Setup(render_graph::RenderGraphBuilder& builder) override;
    virtual void Execute(const render_graph::RGExecutionContext& context) override;

private:
    render_graph::RGResourceHandle target_handle;
    SDL_FColor clear_color;
};
}
