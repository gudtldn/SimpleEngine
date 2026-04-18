#include "SimpleEditor/Picking/PickSubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(PickSubsystem)
    .DependsOn<RenderSubsystem>();

SE_BEGIN_REFLECT(PickSubsystem, meta::Internal)
SE_END_REFLECT(PickSubsystem)

bool PickSubsystem::Initialize()
{
    render_device = &GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();

    // 1x1 R32_UINT pick 텍스처 + readback buffer
    if (!pick_buffer.Create(*render_device))
    {
        return false;
    }

    // 1x1 D24S8 depth 텍스처 (z-test용, 가장 앞 Entity만 pick)
    constexpr SDL_GPUTextureCreateInfo depth_tex_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = 1,
        .height = 1,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    pick_depth_rid = render_device->CreateTexture(depth_tex_info, "Pick_DepthTarget");
    SE_ASSERT_RELEASE(render_device->IsValidTexture(pick_depth_rid));

    return true;
}

void PickSubsystem::Release()
{
    if (render_device->IsValidTexture(pick_depth_rid))
    {
        render_device->DestroyTexture(std::exchange(pick_depth_rid, {}));
    }
    pick_buffer.Destroy();
}

void PickSubsystem::PerformPick()
{
    pick_id = EntityPickId::None();

    if (!pick_buffer)
    {
        return;
    }

    const uint32 raw_encoded = pick_buffer.PerformReadback();
    pick_id = EntityPickId::FromRaw(raw_encoded);
}

SDL_GPUTexture* PickSubsystem::GetPickTexture() const
{
    return pick_buffer.GetTexture();
}

SDL_GPUTexture* PickSubsystem::GetPickDepthTexture() const
{
    if (const auto resource = render_device->GetTexture(pick_depth_rid))
    {
        return resource->handle;
    }
    return nullptr;
}
} // namespace se::editor
