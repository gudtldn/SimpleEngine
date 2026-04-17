#include "SimpleEditor/Picking/GpuPickBuffer.h"

#include <utility>


namespace se::editor
{
bool GpuPickBuffer::Create(graphics::RenderDevice& device)
{
    render_device = &device;

    // 1x1 R32_UINT pick 텍스처
    constexpr SDL_GPUTextureCreateInfo tex_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R32_UINT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        .width = 1,
        .height = 1,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    texture_rid = render_device->CreateTexture(tex_info);
    if (!render_device->IsValidTexture(texture_rid))
    {
        return false;
    }

    // 4바이트 download transfer buffer (GPU -> CPU readback)
    constexpr SDL_GPUTransferBufferCreateInfo tb_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = sizeof(uint32),
    };
    download_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &tb_info);
    return download_buffer != nullptr;
}

void GpuPickBuffer::Destroy()
{
    if (download_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), std::exchange(download_buffer, nullptr));
    }
    if (render_device && render_device->IsValidTexture(texture_rid))
    {
        render_device->DestroyTexture(std::exchange(texture_rid, {}));
    }
}

uint32 GpuPickBuffer::PerformReadback()
{
    if (!IsValid())
    {
        return 0;
    }

    SDL_GPUDevice* raw_device = render_device->GetRawDevice();
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(raw_device);
    if (!cmd)
    {
        return 0;
    }

    SDL_GPUTexture* texture = GetTexture();
    if (!texture)
    {
        SDL_CancelGPUCommandBuffer(cmd);
        return 0;
    }

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    {
        const SDL_GPUTextureRegion src = {
            .texture = texture,
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
    uint32 result = 0;
    if (const void* data = SDL_MapGPUTransferBuffer(raw_device, download_buffer, false))
    {
        result = *static_cast<const uint32*>(data);
        SDL_UnmapGPUTransferBuffer(raw_device, download_buffer);
    }
    return result;
}

SDL_GPUTexture* GpuPickBuffer::GetTexture() const
{
    if (const auto resource = render_device->GetTexture(texture_rid))
    {
        return resource->handle;
    }
    return nullptr;
}
} // namespace se::editor
