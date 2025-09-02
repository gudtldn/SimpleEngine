module;
#include <imgui.h>
#include <imgui_impl_sdlgpu3.h>
module SE.Editor.Rendering;
import :RenderPass.ImGuiPass;

using namespace se::rendering::render_graph;


namespace se::editor::rendering::passes
{
void ImGuiPass::Setup(RenderGraphBuilder& builder)
{
    back_buffer_opt = builder.FindResource(u8"BackBuffer");
    builder.Write(*back_buffer_opt);
}

void ImGuiPass::Execute(const RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    SDL_GPUTexture* back_buffer = context.GetActualTexture(*back_buffer_opt);
    const SDL_GPUColorTargetInfo target_info = {
        .texture = back_buffer,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .load_op = SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = false,
    };

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd);

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd, &target_info, 1, nullptr);
    {
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd, render_pass);
    }
    SDL_EndGPURenderPass(render_pass);

    // Update and Render additional Platform Windows
    const ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::RenderPlatformWindowsDefault();
    }
}
}
