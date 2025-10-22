#include "Rendering/EditorUIPass.h"
#include "SimpleEngine/Rendering/RenderPass/ForwardScenePass.h"
#include "SimpleEngine/Rendering/RenderGraph/RenderGraph.h"

#include "imgui.h"
#include "imgui_impl_sdlgpu3.h"

using namespace se::rendering;


namespace
{
RGResourceHandle forward_scene_color_handle;
RGResourceHandle back_buffer_handle;
}

namespace se::editor::rendering
{
EditorUIPass::EditorUIPass(uint32 width, uint32 height)
    : scene_color_target_width(width)
    , scene_color_target_height(height)
{
}

void EditorUIPass::Setup(RenderGraphBuilder& builder)
{
    forward_scene_color_handle = builder.GetResourceHandleByName(ForwardScenePass::SceneColorTarget);
    builder.Read(forward_scene_color_handle);

    back_buffer_handle = builder.GetResourceHandleByName(u8"BackBuffer");
    builder.Write(back_buffer_handle);
}

void EditorUIPass::Execute(RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* scene_color_texture = context.GetActualTexture(forward_scene_color_handle);
    SDL_GPUTexture* back_buffer = context.GetActualTexture(back_buffer_handle);

    if (scene_color_texture)
    {
        const SDL_GPUBlitInfo blit_info{
            .source = {
                .texture = scene_color_texture,
                .x = 0,
                .y = 0,
                .w = scene_color_target_width,
                .h = scene_color_target_height
            },
            .destination = {
                .texture = back_buffer,
                .x = 0,
                .y = 0,
                .w = scene_color_target_width,
                .h = scene_color_target_height
            },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .flip_mode = SDL_FLIP_NONE,
            .filter = SDL_GPU_FILTER_LINEAR,
        };
        SDL_BlitGPUTexture(cmd, &blit_info);
    }

    const SDL_GPUColorTargetInfo target_info = {
        .texture = back_buffer,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .clear_color = { 0.2f, 0.2f, 0.2f, 1.0f },
        .load_op = scene_color_texture ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_CLEAR,
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
}
