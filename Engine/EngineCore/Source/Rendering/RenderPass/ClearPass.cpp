module SE.Rendering;
import :RenderPass.ClearPass;


namespace se::rendering::passes
{
ClearPass::ClearPass(render_graph::RGResourceHandle target, const SDL_FColor& color)
    : target_handle(target)
    , clear_color(color)
{
}

void ClearPass::Setup(render_graph::RenderGraphBuilder& builder)
{
    builder.Write(target_handle);
}

void ClearPass::Execute(const render_graph::RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();
    SDL_GPUTexture* target_texture = context.GetActualTexture(target_handle);

    if (target_texture == nullptr)
    {
        return;
    }

    SDL_GPUColorTargetInfo color_attachment_info{};
    color_attachment_info.texture = target_texture;
    color_attachment_info.clear_color = clear_color;
    color_attachment_info.load_op = SDL_GPU_LOADOP_CLEAR;   // 이전 내용을 버리고 클리어
    color_attachment_info.store_op = SDL_GPU_STOREOP_STORE; // 작업 결과를 저장

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd, &color_attachment_info, 1, nullptr);
    {
    }
    SDL_EndGPURenderPass(render_pass);
}
}
