#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Memory/MemoryStats.h"
#include "SimpleEngine/Graphics/Memory/GpuMemoryCalc.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
RenderDevice::RenderDevice(SDL_GPUDevice* raw_device)
    : raw_device(raw_device)
{
}

RenderDevice::~RenderDevice()
{
    // 살아 있는 리소스를 모두 Pending Queue로 이동
    Array<RID> live_textures;
    textures.ForEach([&live_textures](RID rid, const TextureResource&)
    {
        live_textures.Push(rid);
    });
    for (const RID rid : live_textures)
    {
        DestroyTexture(rid);
    }

    Array<RID> live_buffers;
    buffers.ForEach([&live_buffers](RID rid, const BufferResource&)
    {
        live_buffers.Push(rid);
    });
    for (const RID rid : live_buffers)
    {
        DestroyBuffer(rid);
    }

    // 지연 파괴 큐를 일괄 처리
    ProcessDeferredDestructions();

    // SDL_GPUDevice 정리
    SDL_DestroyGPUDevice(raw_device);
}

RID RenderDevice::CreateTexture(
    const SDL_GPUTextureCreateInfo& desc,
    [[maybe_unused]] const char* debug_name
)
{
    SDL_GPUTexture* raw = SDL_CreateGPUTexture(raw_device, &desc);
    if (!raw)
    {
        ConsoleLog(ELogLevel::Error, "SDL_CreateGPUTexture failed: {}", SDL_GetError());
        return {};
    }

#if SE_ENABLE_DEBUG_TOOLS
    // props에 이름이 설정되지 않은 경우에만 debug_name을 사용
    if (debug_name && !(desc.props && SDL_GetStringProperty(desc.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, nullptr)))
    {
        SDL_SetGPUTextureName(raw_device, raw, debug_name);
    }
#endif

#if SE_ENABLE_MEMORY_TRACKING
    const u64 byte_size = CalculateTextureMemoryFromCreateInfo(desc);
    const u32 tag_id = MemoryStats::GetCurrentTag();
    MemoryStats::TrackGpuAlloc(tag_id, byte_size);
#endif

    return textures.Insert({
        .handle = raw,
        .width = desc.width,
        .height = desc.height,
        .format = desc.format,
#if SE_ENABLE_MEMORY_TRACKING
        .byte_size = byte_size,
        .tag_id = tag_id,
#endif
    });
}

RID RenderDevice::CreateBuffer(
    const SDL_GPUBufferCreateInfo& desc,
    [[maybe_unused]] const char* debug_name
)
{
    SDL_GPUBuffer* raw = SDL_CreateGPUBuffer(raw_device, &desc);
    if (!raw)
    {
        ConsoleLog(ELogLevel::Error, "SDL_CreateGPUBuffer failed: {}", SDL_GetError());
        return {};
    }

#if SE_ENABLE_DEBUG_TOOLS
    // props에 이름이 설정되지 않은 경우에만 debug_name을 사용
    if (debug_name && !(desc.props && SDL_GetStringProperty(desc.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, nullptr)))
    {
        SDL_SetGPUBufferName(raw_device, raw, debug_name);
    }
#endif

#if SE_ENABLE_MEMORY_TRACKING
    const u32 tag_id = MemoryStats::GetCurrentTag();
    MemoryStats::TrackGpuAlloc(tag_id, desc.size);
#endif

    return buffers.Insert({
        .handle = raw,
        .size = desc.size,
        .usage = desc.usage,
#if SE_ENABLE_MEMORY_TRACKING
        .tag_id = tag_id,
#endif
    });
}

Optional<TextureResource> RenderDevice::GetTexture(RID rid) const
{
    return textures.Get(rid).Copy();
}

Optional<BufferResource> RenderDevice::GetBuffer(RID rid) const
{
    return buffers.Get(rid).Copy();
}

bool RenderDevice::IsValidTexture(RID rid) const
{
    return textures.IsValidRID(rid);
}

bool RenderDevice::IsValidBuffer(RID rid) const
{
    return buffers.IsValidRID(rid);
}

void RenderDevice::DestroyTexture(RID rid)
{
    if (const auto resource = textures.Get(rid))
    {
        deferred_texture_destroys.Push({
            .handle = resource->handle,
#if SE_ENABLE_MEMORY_TRACKING
            .byte_size = resource->byte_size,
            .tag_id = resource->tag_id,
#endif
        });
        textures.Remove(rid);
    }
}

void RenderDevice::DestroyBuffer(RID rid)
{
    if (const auto resource = buffers.Get(rid))
    {
        deferred_buffer_destroys.Push({
            .handle = resource->handle,
            .size = resource->size,
#if SE_ENABLE_MEMORY_TRACKING
            .tag_id = resource->tag_id,
#endif
        });
        buffers.Remove(rid);
    }
}

void RenderDevice::ProcessDeferredDestructions()
{
    for (const PendingTextureDestroy& pending : deferred_texture_destroys)
    {
        SDL_ReleaseGPUTexture(raw_device, pending.handle);
#if SE_ENABLE_MEMORY_TRACKING
        MemoryStats::TrackGpuFree(pending.tag_id, pending.byte_size);
#endif
    }
    deferred_texture_destroys.Clear();

    for (const PendingBufferDestroy& pending : deferred_buffer_destroys)
    {
        SDL_ReleaseGPUBuffer(raw_device, pending.handle);
#if SE_ENABLE_MEMORY_TRACKING
        MemoryStats::TrackGpuFree(pending.tag_id, pending.size);
#endif
    }
    deferred_buffer_destroys.Clear();
}
} // namespace se
