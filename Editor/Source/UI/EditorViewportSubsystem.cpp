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
                render_device->DestroyTexture(info.color_texture);
            }
        }
    }
    viewport_data.Clear();
    render_device = nullptr;
}

void EditorViewportSubsystem::UpdateViewportSize(const StringName& viewport_id, uint32 new_width, uint32 new_height)
{
    if (new_width == 0 || new_height == 0)
    {
        if (const Optional data_opt = viewport_data.Find(viewport_id))
        {
            ViewportRenderInfo& info = data_opt.Value();
            if (info.color_texture)
            {
                render_device->DestroyTexture(
                    std::exchange(info.color_texture, {})
                );
            }
        }
        return;
    }

    // 텍스처가 없거나, 크기가 변경된경우 재생성
    ViewportRenderInfo& info = viewport_data[viewport_id];
    if (
        !info.color_texture.IsValid()
        || info.render_view.width != new_width
        || info.render_view.height != new_height
    )
    {
        if (info.color_texture)
        {
            render_device->DestroyTexture(info.color_texture);
        }

        info.color_texture = render_device->CreateTexture({
            .type = SDL_GPU_TEXTURETYPE_2D,                      // 2D 텍스처
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB, // RGBA 각 채널당 8비트, sRGB 색 공간 사용
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = new_width,
            .height = new_height,
            .layer_count_or_depth = 1, // 2D 텍스처이므로 레이어는 1개
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        });
        info.render_view = {
            // TODO: 카메라 세팅하기
            .view_matrix = Matrix4x4::Identity(),
            .projection_matrix = Matrix4x4::Identity(),
            .color_target_name = viewport_id,
            .depth_target_name = String::Format("{}_Depth", viewport_id),
            .width = new_width,
            .height = new_height,
        };
    }
}

void* EditorViewportSubsystem::GetViewportTextureID(const StringName& viewport_id) const
{
    if (const Optional data_opt = viewport_data.Find(viewport_id))
    {
        const ViewportRenderInfo& info = data_opt.Value();
        if (info.color_texture.IsValid())
        {
            if (const auto tex_resource = render_device->GetTexture(info.color_texture))
            {
                return tex_resource->handle;
            }
        }
    }
    return nullptr;
}
} // namespace se::editor
