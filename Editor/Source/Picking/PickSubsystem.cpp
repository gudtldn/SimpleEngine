#include "SimpleEditor/Picking/PickSubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include <utility>


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(PickSubsystem)
    .DependsOn<RenderSubsystem>();

SE_BEGIN_REFLECT(PickSubsystem, meta::Internal)
SE_END_REFLECT(PickSubsystem)

bool PickSubsystem::Initialize()
{
    render_device = &GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();

    // 4바이트 download transfer buffer (GPU -> CPU readback)
    constexpr SDL_GPUTransferBufferCreateInfo tb_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = sizeof(uint32),
    };
    download_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &tb_info);
    return download_buffer != nullptr;
}

void PickSubsystem::Release()
{
    if (download_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), std::exchange(download_buffer, nullptr));
    }
    if (render_device->IsValidTexture(entity_id_texture_rid))
    {
        render_device->DestroyTexture(std::exchange(entity_id_texture_rid, {}));
    }
}

void PickSubsystem::EnsureSize(uint32 width, uint32 height)
{
    if (texture_width == width && texture_height == height)
    {
        return;
    }

    // 기존 텍스처 해제
    if (render_device->IsValidTexture(entity_id_texture_rid))
    {
        render_device->DestroyTexture(std::exchange(entity_id_texture_rid, {}));
    }

    const SDL_GPUTextureCreateInfo tex_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R32_UINT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    entity_id_texture_rid = render_device->CreateTexture(tex_info, "Pick_EntityID");
    SE_ASSERT_RELEASE(render_device->IsValidTexture(entity_id_texture_rid));

    texture_width = width;
    texture_height = height;
}

void PickSubsystem::PerformPick(const Vector2f& cursor_pos)
{
    pick_id = EntityPickId::None();

    SDL_GPUTexture* texture = GetEntityIdTexture();
    if (!texture || !download_buffer)
    {
        return;
    }

    const uint32 cx = static_cast<uint32>(cursor_pos.x);
    const uint32 cy = static_cast<uint32>(cursor_pos.y);

    // 범위 외 좌표는 무시
    if (cx >= texture_width || cy >= texture_height)
    {
        return;
    }

    SDL_GPUDevice* raw_device = render_device->GetRawDevice();
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(raw_device);
    if (!cmd)
    {
        return;
    }

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    {
        const SDL_GPUTextureRegion src = {
            .texture = texture,
            .x = cx, .y = cy,
            .w = 1, .h = 1, .d = 1,
        };
        const SDL_GPUTextureTransferInfo dst = {
            .transfer_buffer = download_buffer,
            .offset = 0,
        };
        SDL_DownloadFromGPUTexture(copy, &src, &dst);
    }
    SDL_EndGPUCopyPass(copy);

    // Submit + fence 대기 (동기 readback)
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(raw_device, true, &fence, 1);
    SDL_ReleaseGPUFence(raw_device, fence);

    // Transfer buffer 매핑하여 값 읽기
    if (const void* data = SDL_MapGPUTransferBuffer(raw_device, download_buffer, false))
    {
        const uint32 raw_encoded = *static_cast<const uint32*>(data);
        SDL_UnmapGPUTransferBuffer(raw_device, download_buffer);
        pick_id = EntityPickId::FromRaw(raw_encoded);
    }
}

SDL_GPUTexture* PickSubsystem::GetEntityIdTexture() const
{
    if (const auto resource = render_device->GetTexture(entity_id_texture_rid))
    {
        return resource->handle;
    }
    return nullptr;
}
} // namespace se::editor
