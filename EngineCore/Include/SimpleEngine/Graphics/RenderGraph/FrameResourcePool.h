#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Memory/MemoryConfig.h"
#include "SimpleEngine/Core/Memory/MemoryStats.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoEquals.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoHash.h"
#include "SimpleEngine/Utility/Debug.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
// forward declaration
class RenderDevice;

/**
 * 렌더링 파이프라인에서 일시적으로 필요한 텍스처를 재사용하기 위한 Pool
 * 풀링된 리소스는 idle_frames를 추적하여, 일정 프레임 동안 미사용 시 Trim()을 통해 자동 해제됩니다.
 */
class SE_CORE_API FrameResourcePool
{
public:
    /** Pool에 보관 중인 개별 리소스와 idle 프레임 수를 추적하기 위한 구조체 (구현 디테일) */
    template <typename T>
    struct PooledResource
    {
        T* resource = nullptr;
        u32 idle_frames = 0;

#if SE_ENABLE_MEMORY_TRACKING
        // VRAM 추적용 크기 및 생성 시점의 메모리 태그
        u64 byte_size = 0;
        u32 tag_id = 0;
#endif
    };

    /** 특정 CreateInfo에 대한 Pool Entry (구현 디테일) */
    template <typename T>
    struct PoolEntry
    {
        Array<PooledResource<T>> available_resources; // 대기 중 (idle 추적)
        Array<PooledResource<T>> used_resources;      // 현재 프레임에서 사용 중
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

    /** 대기 중인 모든 리소스의 idle 카운터를 1 증가시킵니다. 프레임 경계에서 호출합니다. */
    void IncrementIdleCounters();

    /** max_idle_frames 이상 미사용된 대기 리소스를 실제 GPU 리소스로부터 해제합니다. */
    void Trim(u32 max_idle_frames);

private:
    RenderDevice* render_device;

    HashMap<SDL_GPUTextureCreateInfo, PoolEntry<SDL_GPUTexture>> texture_pool;
    HashMap<SDL_GPUBufferCreateInfo, PoolEntry<SDL_GPUBuffer>> buffer_pool;
};
} // namespace se
