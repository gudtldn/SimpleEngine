#include "SimpleEngine/Graphics/RenderGraph/FrameResourcePool.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include <ranges>


namespace se::graphics
{
FrameResourcePool::FrameResourcePool(RenderDevice& in_render_device)
    : render_device(&in_render_device)
{
}

FrameResourcePool::~FrameResourcePool()
{
    for (const PoolEntry<SDL_GPUTexture>& entry : texture_pool | std::views::values)
    {
        for (const auto& pooled : entry.available_resources)
        {
            SDL_ReleaseGPUTexture(render_device->GetRawDevice(), pooled.resource);
        }
        for (SDL_GPUTexture* texture : entry.used_resources)
        {
            SDL_ReleaseGPUTexture(render_device->GetRawDevice(), texture);
        }
    }

    for (const PoolEntry<SDL_GPUBuffer>& entry : buffer_pool | std::views::values)
    {
        for (const auto& pooled : entry.available_resources)
        {
            SDL_ReleaseGPUBuffer(render_device->GetRawDevice(), pooled.resource);
        }
        for (SDL_GPUBuffer* buffer : entry.used_resources)
        {
            SDL_ReleaseGPUBuffer(render_device->GetRawDevice(), buffer);
        }
    }
}

SDL_GPUTexture* FrameResourcePool::AcquireTexture(const SDL_GPUTextureCreateInfo& info)
{
    return AcquireResourceInternal(texture_pool[info], [this, &info = std::as_const(info)]
    {
        return SDL_CreateGPUTexture(render_device->GetRawDevice(), &info);
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
        return SDL_CreateGPUBuffer(render_device->GetRawDevice(), &info);
    });
}

void FrameResourcePool::ReleaseBuffer(const SDL_GPUBufferCreateInfo& info, SDL_GPUBuffer* buffer)
{
    ReleaseResourceInternal(buffer_pool[info], buffer);
}

void FrameResourcePool::IncrementIdleCounters()
{
    auto increment_pool_counters = []<typename K, typename T>(HashMap<K, PoolEntry<T>>& pool)
    {
        for (PoolEntry<T>& entry : pool | std::views::values)
        {
            for (PooledResource<T>& pooled : entry.available_resources)
            {
                ++pooled.idle_frames;
            }
        }
    };

    increment_pool_counters(texture_pool);
    increment_pool_counters(buffer_pool);
}

void FrameResourcePool::Trim(uint32 max_idle_frames)
{
    SDL_GPUDevice* raw_device = render_device->GetRawDevice();

    for (PoolEntry<SDL_GPUTexture>& entry : texture_pool | std::views::values)
    {
        TrimEntry(entry, max_idle_frames, [raw_device](SDL_GPUTexture* texture)
        {
            SDL_ReleaseGPUTexture(raw_device, texture);
        });
    }

    for (PoolEntry<SDL_GPUBuffer>& entry : buffer_pool | std::views::values)
    {
        TrimEntry(entry, max_idle_frames, [raw_device](SDL_GPUBuffer* buffer)
        {
            SDL_ReleaseGPUBuffer(raw_device, buffer);
        });
    }
}
}  // namespace se::graphics
