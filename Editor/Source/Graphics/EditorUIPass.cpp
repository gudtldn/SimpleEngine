#include "Graphics/EditorUIPass.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraph.h"

#include "imgui.h"
#include "imgui_impl_sdlgpu3.h"

using namespace se::graphics;


namespace
{
RGResourceHandle back_buffer_handle;
}

namespace se::editor
{
void EditorUIPass::Setup(RenderGraphBuilder& builder)
{
    back_buffer_handle = builder.GetResourceHandleByName("BackBuffer");
    builder.Write(back_buffer_handle);
}

void EditorUIPass::Execute(RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* back_buffer = context.GetActualTexture(back_buffer_handle);
    const SDL_GPUColorTargetInfo target_info = {
        .texture = back_buffer,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .clear_color = { 0.0f, 0.0f, 0.0f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = false,
    };

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

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
}  // namespace se::editor
