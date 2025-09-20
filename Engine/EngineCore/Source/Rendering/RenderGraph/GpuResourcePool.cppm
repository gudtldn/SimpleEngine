export module SE.Rendering:RenderGraph.GpuResourcePool;
import :Traits.CreateInfoEquals;
import :Traits.CreateInfoHash;

import SE.Types;
import std;

import <cassert>;
import "SDL3/SDL_gpu.h";


class GpuResourcePool
{
private:
    // Pool 관리용 Entry
    template <typename T>
    struct PoolEntry
    {
        se::vector<T*> available_resources;
        se::vector<T*> used_resources;
    };

public:
    explicit GpuResourcePool(SDL_GPUDevice* in_device);
    ~GpuResourcePool();

    GpuResourcePool(const GpuResourcePool&) = delete;
    GpuResourcePool& operator=(const GpuResourcePool&) = delete;
    GpuResourcePool(GpuResourcePool&&) = default;
    GpuResourcePool& operator=(GpuResourcePool&&) = default;

public:
    /** CreateInfo에 맞는 텍스처를 Pool에서 찾거나 새로 생성하여 반환합니다. */
    [[nodiscard]] SDL_GPUTexture* AllocateTexture(const SDL_GPUTextureCreateInfo& info);

    /** 사용이 끝난 텍스처를 Pool에 반납합니다. */
    void DeallocateTexture(const SDL_GPUTextureCreateInfo& info, SDL_GPUTexture* texture);

    /** CreateInfo에 맞는 버퍼를 Pool에서 찾거나 새로 생성하여 반환합니다. */
    [[nodiscard]] SDL_GPUBuffer* AllocateBuffer(const SDL_GPUBufferCreateInfo& info);

    /** 사용이 끝난 버퍼를 Pool에 반납합니다. */
    void DeallocateBuffer(const SDL_GPUBufferCreateInfo& info, SDL_GPUBuffer* buffer);

private:
    template <typename T, typename CreateResourceFn>
        requires std::is_invocable_r_v<T*, CreateResourceFn>
    [[nodiscard]] T* AllocateResource(PoolEntry<T>& entry, CreateResourceFn&& create_resource_func);

    template <typename T>
    void DeallocateResource(PoolEntry<T>& entry, T* resource);

private:
    SDL_GPUDevice* device;
    se::unordered_map<SDL_GPUTextureCreateInfo, PoolEntry<SDL_GPUTexture>> texture_pool;
    se::unordered_map<SDL_GPUBufferCreateInfo, PoolEntry<SDL_GPUBuffer>> buffer_pool;
};

template <typename T, typename CreateResourceFn>
    requires std::is_invocable_r_v<T*, CreateResourceFn>
T* GpuResourcePool::AllocateResource(PoolEntry<T>& entry, CreateResourceFn&& create_resource_func)
{
    if (entry.available_resources.empty())
    {
        entry.used_resources.push_back(create_resource_func());
    }
    else [[likely]]
    {
        entry.used_resources.push_back(entry.available_resources.back());
        entry.available_resources.pop_back();
    }
    return entry.used_resources.back();
}

template <typename T>
void GpuResourcePool::DeallocateResource(PoolEntry<T>& entry, T* resource)
{
    [[maybe_unused]] const size_t count = std::erase(entry.used_resources, resource);
    assert(count > 0 && "Attempted to deallocate a texture that was not marked as used.");

    entry.available_resources.push_back(resource);
}
