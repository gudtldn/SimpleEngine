#include "SimpleEditor/Picking/PickSubsystem.h"
#include "SimpleEditor/Picking/EntityPickId.h"

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

    // 1x1 R32_UINT pick 텍스처 (entity ID 출력용)
    constexpr SDL_GPUTextureCreateInfo color_tex_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R32_UINT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        .width = 1,
        .height = 1,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    pick_texture_rid = render_device->CreateTexture(color_tex_info);
    SE_ASSERT_RELEASE(render_device->IsValidTexture(pick_texture_rid));

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
    pick_depth_rid = render_device->CreateTexture(depth_tex_info);
    SE_ASSERT_RELEASE(render_device->IsValidTexture(pick_depth_rid));

    // 4바이트 download transfer buffer (GPU -> CPU readback)
    constexpr SDL_GPUTransferBufferCreateInfo tb_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = sizeof(uint32),
    };
    download_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &tb_info);
    SE_ASSERT_RELEASE(download_buffer);

    return true;
}

void PickSubsystem::Release()
{
    if (download_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), std::exchange(download_buffer, nullptr));
    }
    if (render_device->IsValidTexture(pick_depth_rid))
    {
        render_device->DestroyTexture(std::exchange(pick_depth_rid, {}));
    }
    if (render_device->IsValidTexture(pick_texture_rid))
    {
        render_device->DestroyTexture(std::exchange(pick_texture_rid, {}));
    }
}

void PickSubsystem::PerformPick()
{
    if (!pick_texture_rid || !download_buffer)
    {
        return;
    }

    // pick 텍스처 -> download transfer buffer 복사
    SDL_GPUDevice* raw_device = render_device->GetRawDevice();
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(raw_device);
    if (!cmd)
    {
        return;
    }

    SDL_GPUTexture* pick_texture = GetPickTexture();
    if (!pick_texture)
    {
        picked_entity_id = Entity::Invalid;
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    {
        const SDL_GPUTextureRegion src = {
            .texture = pick_texture,
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

    // Transfer buffer 매핑 -> encoded entity ID 읽기
    uint32 encoded_id = Entity::Invalid;
    if (const void* data = SDL_MapGPUTransferBuffer(raw_device, download_buffer, false))
    {
        encoded_id = *static_cast<const uint32*>(data);
        SDL_UnmapGPUTransferBuffer(raw_device, download_buffer);
    }

    // Entity::Invalid(0) = GPU clear값 = 빈 공간
    // 디코딩된 entity.id는 Entity::Invalid와 비교하여 유효성 판단
    if (encoded_id != Entity::Invalid)
    {
        picked_entity_id = DecodeEntityPickId(encoded_id);
    }
    else
    {
        picked_entity_id = Entity::Invalid;
    }
}

SDL_GPUTexture* PickSubsystem::GetPickTexture() const
{
    if (const auto resource = render_device->GetTexture(pick_texture_rid))
    {
        return resource->handle;
    }
    return nullptr;
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
