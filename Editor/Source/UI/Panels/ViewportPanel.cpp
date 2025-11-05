#include "UI/Panels/ViewportPanel.h"

#include <cassert>

#include "imgui.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"


namespace se::editor::ui
{
ViewportPanel::~ViewportPanel()
{
    if (viewport_color_texture)
    {
        if (const RenderSubsystem* render_subsystem = utility::GetSubsystemUnchecked<RenderSubsystem>())
        {
            SDL_GPUDevice* device = render_subsystem->GetGpuDevice();

            SDL_ReleaseGPUTexture(device, viewport_color_texture);
            SDL_ReleaseGPUTexture(device, viewport_depth_texture);

            viewport_color_texture = nullptr;
            viewport_depth_texture = nullptr;
        }
    }
}

const char* ViewportPanel::GetName() const
{
    return "Viewport";
}

void ViewportPanel::Draw(EditorUIContext& context)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(GetName(), &is_visible);
    {
        const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
        ResizeViewportTexture(static_cast<uint32>(viewport_size.x), static_cast<uint32>(viewport_size.y));

        if (viewport_color_texture)
        {
            ImGui::Image(viewport_color_texture, viewport_size);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::ResizeViewportTexture(uint32 new_width, uint32 new_height)
{
    if (new_width == 0 || new_height == 0 || (viewport_width == new_width && viewport_height == new_height))
    {
        return;
    }

    const RenderSubsystem* render_subsystem = utility::GetSubsystemUnchecked<RenderSubsystem>();
    assert(render_subsystem && "Render subsystem is not initialized.");

    SDL_GPUDevice* device = render_subsystem->GetGpuDevice();
    assert(device && "GPU device is not initialized.");

    if (viewport_color_texture)
    {
        SDL_ReleaseGPUTexture(device, viewport_color_texture);
        SDL_ReleaseGPUTexture(device, viewport_depth_texture);
    }

    const SDL_GPUTextureCreateInfo color_create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,                      // 2D 텍스처
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB, // RGBA 각 채널당 8비트, sRGB 색 공간 사용
        // 이 텍스처의 사용 목적
        // - COLOR_TARGET: 이 텍스처에 그림을 그릴(렌더링할) 것임을 의미
        // - SAMPLER: 나중에 다른 패스(예: 후처리, UI)에서 이 텍스처를 읽어서 사용할 것임을 의미
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = new_width,
        .height = new_height,
        .layer_count_or_depth = 1, // 2D 텍스처이므로 레이어는 1개
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    viewport_color_texture = SDL_CreateGPUTexture(device, &color_create_info);

    const SDL_GPUTextureCreateInfo depth_create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,  // 24비트 깊이버퍼 + 8비트 스텐실버퍼
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, // 이 텍스처는 깊이/스텐실 버퍼로만 사용될 것임을 의미
        .width = new_width,
        .height = new_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    viewport_depth_texture = SDL_CreateGPUTexture(device, &depth_create_info);

    viewport_width = new_width;
    viewport_height = new_height;
}
}
