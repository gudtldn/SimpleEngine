#include "SimpleEngine/Graphics/RenderGraph/FrameResourcePool.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Graphics/Memory/GpuMemoryCalc.h"

#include <ranges>


namespace se
{
namespace
{
template <typename T, typename FactoryFn>
    requires std::is_invocable_r_v<FrameResourcePool::PooledResource<T>, FactoryFn>
[[nodiscard]] T* AcquireResourceInternal(FrameResourcePool::PoolEntry<T>& entry, FactoryFn&& factory_func)
{
    if (auto pooled = entry.available_resources.Pop())
    {
        pooled->idle_frames = 0;
        entry.used_resources.Push(std::move(pooled).Value());
    }
    else [[unlikely]] // NOLINT(*-inconsistent-ifelse-braces)
    {
        entry.used_resources.Push(std::forward<FactoryFn>(factory_func)());
    }
    return entry.used_resources.Back()->resource;
}

template <typename T>
void ReleaseResourceInternal(FrameResourcePool::PoolEntry<T>& entry, T* resource)
{
    for (usize i = 0; i < entry.used_resources.Len(); ++i)
    {
        if (entry.used_resources[i].resource == resource)
        {
            FrameResourcePool::PooledResource<T> pooled = std::move(entry.used_resources[i]);
            pooled.idle_frames = 0;
            entry.used_resources.RemoveAtSwap(i);
            entry.available_resources.Push(std::move(pooled));
            return;
        }
    }
    SE_ASSERT(false, "Attempted to deallocate a resource that was not marked as used.");
}

template <typename T, typename ReleaseFn>
    requires std::invocable<ReleaseFn, T*>
void TrimEntry(FrameResourcePool::PoolEntry<T>& entry, u32 max_idle_frames, ReleaseFn&& release_fn)
{
    for (isize i = static_cast<isize>(entry.available_resources.Len()) - 1; i >= 0; --i)
    {
        FrameResourcePool::PooledResource<T>& pooled = entry.available_resources[static_cast<usize>(i)];
        if (pooled.idle_frames >= max_idle_frames)
        {
            release_fn(pooled.resource);
#if SE_ENABLE_MEMORY_TRACKING
            MemoryStats::TrackGpuFree(pooled.tag_id, pooled.byte_size);
#endif
            entry.available_resources.RemoveAtSwap(static_cast<usize>(i));
        }
    }
}
} // namespace

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
#if SE_ENABLE_MEMORY_TRACKING
            MemoryStats::TrackGpuFree(pooled.tag_id, pooled.byte_size);
#endif
        }
        for (const auto& pooled : entry.used_resources)
        {
            SDL_ReleaseGPUTexture(render_device->GetRawDevice(), pooled.resource);
#if SE_ENABLE_MEMORY_TRACKING
            MemoryStats::TrackGpuFree(pooled.tag_id, pooled.byte_size);
#endif
        }
    }

    for (const PoolEntry<SDL_GPUBuffer>& entry : buffer_pool | std::views::values)
    {
        for (const auto& pooled : entry.available_resources)
        {
            SDL_ReleaseGPUBuffer(render_device->GetRawDevice(), pooled.resource);
#if SE_ENABLE_MEMORY_TRACKING
            MemoryStats::TrackGpuFree(pooled.tag_id, pooled.byte_size);
#endif
        }
        for (const auto& pooled : entry.used_resources)
        {
            SDL_ReleaseGPUBuffer(render_device->GetRawDevice(), pooled.resource);
#if SE_ENABLE_MEMORY_TRACKING
            MemoryStats::TrackGpuFree(pooled.tag_id, pooled.byte_size);
#endif
        }
    }
}

SDL_GPUTexture* FrameResourcePool::AcquireTexture(const SDL_GPUTextureCreateInfo& info)
{
    SE_MEM_SCOPE("GPU:Transient");
    return AcquireResourceInternal(texture_pool.Entry(info).OrDefault(), [this, &info = std::as_const(info)]
    {
        PooledResource<SDL_GPUTexture> pooled;
        pooled.resource = SDL_CreateGPUTexture(render_device->GetRawDevice(), &info);
#if SE_ENABLE_MEMORY_TRACKING
        if (pooled.resource)
        {
            pooled.byte_size = CalculateTextureMemoryFromCreateInfo(info);
            pooled.tag_id = MemoryStats::GetCurrentTag();
            MemoryStats::TrackGpuAlloc(pooled.tag_id, pooled.byte_size);
        }
#endif
        return pooled;
    });
}

void FrameResourcePool::ReleaseTexture(const SDL_GPUTextureCreateInfo& info, SDL_GPUTexture* texture)
{
    ReleaseResourceInternal(texture_pool.Entry(info).OrDefault(), texture);
}

SDL_GPUBuffer* FrameResourcePool::AcquireBuffer(const SDL_GPUBufferCreateInfo& info)
{
    SE_MEM_SCOPE("GPU:Transient");
    return AcquireResourceInternal(buffer_pool.Entry(info).OrDefault(), [this, &info = std::as_const(info)]
    {
        PooledResource<SDL_GPUBuffer> pooled;
        pooled.resource = SDL_CreateGPUBuffer(render_device->GetRawDevice(), &info);
#if SE_ENABLE_MEMORY_TRACKING
        if (pooled.resource)
        {
            pooled.byte_size = info.size;
            pooled.tag_id = MemoryStats::GetCurrentTag();
            MemoryStats::TrackGpuAlloc(pooled.tag_id, pooled.byte_size);
        }
#endif
        return pooled;
    });
}

void FrameResourcePool::ReleaseBuffer(const SDL_GPUBufferCreateInfo& info, SDL_GPUBuffer* buffer)
{
    ReleaseResourceInternal(buffer_pool.Entry(info).OrDefault(), buffer);
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

void FrameResourcePool::Trim(u32 max_idle_frames)
{
    SDL_GPUDevice* raw_device = render_device->GetRawDevice();

    auto trim_pool_internal = [max_idle_frames]<typename Key, typename Resource, typename ReleaseFn>(
        HashMap<Key, PoolEntry<Resource>>& pool,
        ReleaseFn&& release_fn
    )
    {
        Array<Key> keys_to_remove;
        for (auto& [info, entry] : pool)
        {
            // Pool Entry의 사용하지 않는 Resource 정리
            TrimEntry(entry, max_idle_frames, std::forward<ReleaseFn>(release_fn));

            if (entry.available_resources.IsEmpty() && entry.used_resources.IsEmpty())
            {
                keys_to_remove.Push(info);
            }
        }

        // 이제 더 이상 사용하지 않는 Entry를 정리
        for (const Key& info : keys_to_remove)
        {
            pool.Remove(info);
        }
    };

    // Texture Pool 정리
    trim_pool_internal(texture_pool, [raw_device](SDL_GPUTexture* texture)
    {
        SDL_ReleaseGPUTexture(raw_device, texture);
    });

    // Buffer Pool 정리
    trim_pool_internal(buffer_pool, [raw_device](SDL_GPUBuffer* buffer)
    {
        SDL_ReleaseGPUBuffer(raw_device, buffer);
    });
}
} // namespace se
