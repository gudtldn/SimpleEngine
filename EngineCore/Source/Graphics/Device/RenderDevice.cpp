#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
RenderDevice::RenderDevice(SDL_GPUDevice* raw_device)
    : raw_device(raw_device)
{
}

RenderDevice::~RenderDevice()
{
    // 지연 파괴 큐를 먼저 처리
    ProcessDeferredDestructions();

    // 남아 있는 모든 라이브 리소스를 해제
    textures.ForEach([this](RID, const TextureResource& resource)
    {
        SDL_ReleaseGPUTexture(raw_device, resource.handle);
    });
    textures.Clear();

    buffers.ForEach([this](RID, const BufferResource& resource)
    {
        SDL_ReleaseGPUBuffer(raw_device, resource.handle);
    });
    buffers.Clear();
}

RID RenderDevice::CreateTexture(const SDL_GPUTextureCreateInfo& desc)
{
    SDL_GPUTexture* raw = SDL_CreateGPUTexture(raw_device, &desc);
    if (!raw)
    {
        ConsoleLog(ELogLevel::Error, "SDL_CreateGPUTexture failed: {}", SDL_GetError());
        return {};
    }

    return textures.Insert({
        .handle = raw,
        .width = desc.width,
        .height = desc.height,
        .format = desc.format,
    });
}

RID RenderDevice::CreateBuffer(const SDL_GPUBufferCreateInfo& desc)
{
    SDL_GPUBuffer* raw = SDL_CreateGPUBuffer(raw_device, &desc);
    if (!raw)
    {
        ConsoleLog(ELogLevel::Error, "SDL_CreateGPUBuffer failed: {}", SDL_GetError());
        return {};
    }

    return buffers.Insert({
        .handle = raw,
        .size = desc.size,
        .usage = desc.usage,
    });
}

Optional<const TextureResource&> RenderDevice::GetTexture(RID rid) const
{
    return textures.Get(rid);
}

Optional<const BufferResource&> RenderDevice::GetBuffer(RID rid) const
{
    return buffers.Get(rid);
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
    if (auto resource = textures.Get(rid))
    {
        deferred_texture_destroys.Push(resource->handle);
        textures.Remove(rid);
    }
}

void RenderDevice::DestroyBuffer(RID rid)
{
    if (auto resource = buffers.Get(rid))
    {
        deferred_buffer_destroys.Push(resource->handle);
        buffers.Remove(rid);
    }
}

void RenderDevice::ProcessDeferredDestructions()
{
    for (SDL_GPUTexture* texture : deferred_texture_destroys)
    {
        SDL_ReleaseGPUTexture(raw_device, texture);
    }
    deferred_texture_destroys.Clear();

    for (SDL_GPUBuffer* buffer : deferred_buffer_destroys)
    {
        SDL_ReleaseGPUBuffer(raw_device, buffer);
    }
    deferred_buffer_destroys.Clear();
}
} // namespace se::graphics
