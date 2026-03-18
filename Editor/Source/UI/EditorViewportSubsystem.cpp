#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(EditorViewportSubsystem)
    .DependsOn<RenderSubsystem>();

SE_BEGIN_REFLECT(EditorViewportSubsystem, meta::Internal)
SE_END_REFLECT(EditorViewportSubsystem)

bool EditorViewportSubsystem::Initialize()
{
    render_device = &GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();
    return true;
}

void EditorViewportSubsystem::Release()
{
    if (render_device)
    {
        for (const ViewportRenderInfo& info : viewport_data | std::views::values)
        {
            if (info.color_texture)
            {
                SDL_ReleaseGPUTexture(render_device->GetRawDevice(), info.color_texture);
            }
        }
    }
    viewport_data.Clear();
    render_device = nullptr;
}

SDL_GPUTexture* EditorViewportSubsystem::UpdateAndGetViewportTexture(const StringName& viewport_id, uint32 new_width, uint32 new_height)
{
    if (new_width == 0 || new_height == 0 )
    {
        if (Optional data_opt = viewport_data.Find(viewport_id))
        {
            ViewportRenderInfo& info = data_opt.Value();
            if (info.color_texture)
            {
                SDL_ReleaseGPUTexture(render_device->GetRawDevice(), info.color_texture);
                info.color_texture = nullptr;
            }
            return nullptr;
        }
    }

    // 텍스처가 없거나, 크기가 변경된경우 재생성
    ViewportRenderInfo& info = viewport_data[viewport_id];
    if (info.color_texture == nullptr || info.width != new_width || info.height != new_height)
    {
        if (info.color_texture)
        {
            SDL_ReleaseGPUTexture(render_device->GetRawDevice(), info.color_texture);
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
        info.color_texture = SDL_CreateGPUTexture(render_device->GetRawDevice(), &color_create_info);
        info.width = new_width;
        info.height = new_height;
    }
    return info.color_texture;
}
}  // namespace se::editor
