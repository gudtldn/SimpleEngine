#include "Rendering/RenderGraph/GpuResourcePool.h"

#include <ranges>


namespace se::rendering
{
GpuResourcePool::GpuResourcePool(SDL_GPUDevice* in_device)
    : device(in_device)
{
}

GpuResourcePool::~GpuResourcePool()
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

SDL_GPUTexture* GpuResourcePool::AllocateTexture(const SDL_GPUTextureCreateInfo& info)
{
    return AllocateResource(texture_pool[info], [this, &info = std::as_const(info)]
    {
        return SDL_CreateGPUTexture(device, &info);
    });
}

void GpuResourcePool::DeallocateTexture(const SDL_GPUTextureCreateInfo& info, SDL_GPUTexture* texture)
{
    DeallocateResource(texture_pool[info], texture);
}

SDL_GPUBuffer* GpuResourcePool::AllocateBuffer(const SDL_GPUBufferCreateInfo& info)
{
    return AllocateResource(buffer_pool[info], [this, &info = std::as_const(info)]
    {
        return SDL_CreateGPUBuffer(device, &info);
    });
}

void GpuResourcePool::DeallocateBuffer(const SDL_GPUBufferCreateInfo& info, SDL_GPUBuffer* buffer)
{
    DeallocateResource(buffer_pool[info], buffer);
}
}
