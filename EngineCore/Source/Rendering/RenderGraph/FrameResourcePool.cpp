#include "Rendering/RenderGraph/FrameResourcePool.h"

#include <ranges>


namespace se::graphics
{
FrameResourcePool::FrameResourcePool(SDL_GPUDevice* in_device)
    : device(in_device)
{
}

FrameResourcePool::~FrameResourcePool()
{
    for (const PoolEntry<SDL_GPUTexture>& entry : texture_pool | std::views::values)
    {
        for (SDL_GPUTexture* texture : entry.available_resources)
        {
            SDL_ReleaseGPUTexture(device, texture);
        }
        for (SDL_GPUTexture* texture : entry.used_resources)
        {
            SDL_ReleaseGPUTexture(device, texture);
        }
    }

    for (const PoolEntry<SDL_GPUBuffer>& entry : buffer_pool | std::views::values)
    {
        for (SDL_GPUBuffer* buffer : entry.available_resources)
        {
            SDL_ReleaseGPUBuffer(device, buffer);
        }
        for (SDL_GPUBuffer* buffer : entry.used_resources)
        {
            SDL_ReleaseGPUBuffer(device, buffer);
        }
    }
}

SDL_GPUTexture* FrameResourcePool::AcquireTexture(const SDL_GPUTextureCreateInfo& info)
{
    return AcquireResourceInternal(texture_pool[info], [this, &info = std::as_const(info)]
    {
        return SDL_CreateGPUTexture(device, &info);
    });
}

void FrameResourcePool::ReleaseTexture(const SDL_GPUTextureCreateInfo& info, SDL_GPUTexture* texture)
{
    ReleaseResourceInternal(texture_pool[info], texture);
}

SDL_GPUBuffer* FrameResourcePool::AcquireBuffer(const SDL_GPUBufferCreateInfo& info)
{
    return AcquireResourceInternal(buffer_pool[info], [this, &info = std::as_const(info)]
    {
        return SDL_CreateGPUBuffer(device, &info);
    });
}

void FrameResourcePool::ReleaseBuffer(const SDL_GPUBufferCreateInfo& info, SDL_GPUBuffer* buffer)
{
    ReleaseResourceInternal(buffer_pool[info], buffer);
}
}  // namespace se::graphics
