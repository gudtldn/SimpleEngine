#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoEquals.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoHash.h"
#include "SimpleEngine/Utility/Debug.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
// forward declaration
class RenderDevice;

/**
 * 렌더링 파이프라인에서 일시적으로 필요한 텍스처를 재사용하기 위한 Pool
 */
class SE_CORE_API FrameResourcePool
{
private:
    // Pool 관리용 Entry
    template <typename T>
    struct PoolEntry
    {
        Array<T*> available_resources;
        Array<T*> used_resources;
    };

public:
    explicit FrameResourcePool(RenderDevice& in_render_device);
    ~FrameResourcePool();

    FrameResourcePool(const FrameResourcePool&) = delete;
    FrameResourcePool& operator=(const FrameResourcePool&) = delete;
    FrameResourcePool(FrameResourcePool&&) noexcept = default;
    FrameResourcePool& operator=(FrameResourcePool&&) noexcept = default;

public:
    /** 렌더링에 필요한 임시 텍스처(G-Buffer 등)를 요청합니다. */
    [[nodiscard]] SDL_GPUTexture* AcquireTexture(const SDL_GPUTextureCreateInfo& info);

    /** 프레임 종료 후 사용된 텍스처를 반납합니다. */
    void ReleaseTexture(const SDL_GPUTextureCreateInfo& info, SDL_GPUTexture* texture);

    /** Compute Shader 등을 위한 임시 버퍼를 요청합니다. */
    [[nodiscard]] SDL_GPUBuffer* AcquireBuffer(const SDL_GPUBufferCreateInfo& info);

    /** 프레임 종료 후 사용된 버퍼를 반납합니다. */
    void ReleaseBuffer(const SDL_GPUBufferCreateInfo& info, SDL_GPUBuffer* buffer);

    /** 사용되지 않은 리소스를 모두 제거합니다. | TODO: PruneUnusedResources 구현하기 */
    // void PruneUnusedResources();

private:
    template <typename T, typename CreateResourceFn>
        requires std::is_invocable_r_v<T*, CreateResourceFn>
    [[nodiscard]] static T* AcquireResourceInternal(PoolEntry<T>& entry, CreateResourceFn&& factory_func);

    template <typename T>
    static void ReleaseResourceInternal(PoolEntry<T>& entry, T* resource);

private:
    RenderDevice* render_device;

    HashMap<SDL_GPUTextureCreateInfo, PoolEntry<SDL_GPUTexture>> texture_pool;
    HashMap<SDL_GPUBufferCreateInfo, PoolEntry<SDL_GPUBuffer>> buffer_pool;
};

template <typename T, typename FactoryFn>
    requires std::is_invocable_r_v<T*, FactoryFn>
T* FrameResourcePool::AcquireResourceInternal(PoolEntry<T>& entry, FactoryFn&& factory_func)
{
    if (Optional resource_opt = entry.available_resources.Pop())
    {
        entry.used_resources.Push(*resource_opt);
    }
    else [[unlikely]]
    {
        entry.used_resources.Push(std::forward<FactoryFn>(factory_func)());
    }
    return *entry.used_resources.Back();
}

template <typename T>
void FrameResourcePool::ReleaseResourceInternal(PoolEntry<T>& entry, T* resource)
{
    const Optional<usize> remove_idx_opt = entry.used_resources.Find(resource);
    SE_ASSERT(remove_idx_opt.HasValue(), "Attempted to deallocate a texture that was not marked as used.");

    entry.used_resources.RemoveAtSwap(*remove_idx_opt);
    entry.available_resources.Push(resource);
}
} // namespace se::graphics
