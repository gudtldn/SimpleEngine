export module SE.Rendering:RenderGraph.GpuResourcePool;
import :Traits.CreateInfoEquals;
import :Traits.CreateInfoHash;

import SE.Types;
import std;

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

    /** 프레임 시작 시 호출되어, 지난 프레임에 사용된 모든 리소스를 '사용 가능' 상태로 되돌립니다. */
    void BeginFrame();

    /** CreateInfo에 맞는 텍스처를 Pool에서 찾거나 새로 생성하여 반환합니다. */
    [[nodiscard]] SDL_GPUTexture* AllocateTexture(const SDL_GPUTextureCreateInfo& info);

    /** CreateInfo에 맞는 버퍼를 Pool에서 찾거나 새로 생성하여 반환합니다. */
    [[nodiscard]] SDL_GPUBuffer* AllocateBuffer(const SDL_GPUBufferCreateInfo& info);

private:
    template <typename T, typename CreateResourceFn>
        requires std::is_invocable_r_v<T*, CreateResourceFn>
    [[nodiscard]] T* AllocateResource(PoolEntry<T>& entry, CreateResourceFn&& create_resource_func);

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
