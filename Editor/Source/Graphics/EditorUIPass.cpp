#include "Graphics/EditorUIPass.h"

#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"

#include "imgui.h"
#include "imgui_impl_sdlgpu3.h"


namespace se::editor
{
using namespace se::graphics;

SE_BEGIN_REFLECT(EditorUIPass, meta::Internal)
SE_END_REFLECT(EditorUIPass)

void EditorUIPass::Setup(RGSetupContext& context)
{
    context.Write(swapchain_handle);
}

void EditorUIPass::Execute(RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* swapchain_texture = context.GetActualTexture(swapchain_handle);
    const SDL_GPUColorTargetInfo target_info = {
        .texture = swapchain_texture,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
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
} // namespace se::editor
