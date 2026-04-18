#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Graphics/Device/RID.h"
#include "SimpleEngine/Graphics/Device/SlotMap.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
/** RenderDevice가 관리하는 텍스처 리소스의 메타데이터입니다. */
struct TextureResource
{
    SDL_GPUTexture* handle = nullptr;
    uint32 width = 0;
    uint32 height = 0;
    SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_INVALID;
};

/** RenderDevice가 관리하는 버퍼 리소스의 메타데이터입니다. */
struct BufferResource
{
    SDL_GPUBuffer* handle = nullptr;
    uint32 size = 0;
    SDL_GPUBufferUsageFlags usage = 0;
};


/**
 * SDL_GPUDevice를 래핑하여 GPU 리소스를 RID 기반으로 관리하는 클래스
 *
 * 모든 GPU 리소스(텍스처, 버퍼)는 SlotMap에 저장되며, RID를 통해 O(1) 접근합니다.
 * 리소스 삭제는 지연 파괴(deferred destruction) 큐를 통해 프레임 경계에서 일괄 처리됩니다.
 */
class SE_CORE_API RenderDevice
{
public:
    /** @param raw_device SDL_GPUDevice 포인터입니다. */
    explicit RenderDevice(SDL_GPUDevice* raw_device);
    ~RenderDevice();

    RenderDevice(const RenderDevice&) = delete;
    RenderDevice& operator=(const RenderDevice&) = delete;
    RenderDevice(RenderDevice&&) = delete;
    RenderDevice& operator=(RenderDevice&&) = delete;

public:
    /** GPU 텍스처를 생성하고 RID를 반환합니다. 실패 시 유효하지 않은 RID를 반환합니다. */
    [[nodiscard]] RID CreateTexture(const SDL_GPUTextureCreateInfo& desc, const char* debug_name = nullptr);

    /** GPU 버퍼를 생성하고 RID를 반환합니다. 실패 시 유효하지 않은 RID를 반환합니다. */
    [[nodiscard]] RID CreateBuffer(const SDL_GPUBufferCreateInfo& desc, const char* debug_name = nullptr);

    /** RID에 해당하는 텍스처 리소스를 반환합니다. 유효하지 않은 RID이면 NullOpt를 반환합니다. */
    [[nodiscard]] Optional<const TextureResource&> GetTexture(RID rid) const;

    /** RID에 해당하는 버퍼 리소스를 반환합니다. 유효하지 않은 RID이면 NullOpt를 반환합니다. */
    [[nodiscard]] Optional<const BufferResource&> GetBuffer(RID rid) const;

    /** RID가 유효한 텍스처를 가리키는지 확인합니다. */
    [[nodiscard]] bool IsValidTexture(RID rid) const;

    /** RID가 유효한 버퍼를 가리키는지 확인합니다. */
    [[nodiscard]] bool IsValidBuffer(RID rid) const;

    /**
     * 텍스처를 지연 파괴 큐에 등록합니다.
     * RID는 즉시 무효화되지만, 실제 GPU 리소스 해제는 ProcessDeferredDestructions() 호출 시 수행됩니다.
     */
    void DestroyTexture(RID rid);

    /**
     * 버퍼를 지연 파괴 큐에 등록합니다.
     * RID는 즉시 무효화되지만, 실제 GPU 리소스 해제는 ProcessDeferredDestructions() 호출 시 수행됩니다.
     */
    void DestroyBuffer(RID rid);

    /**
     * 지연 파괴 큐에 등록된 모든 GPU 리소스를 실제로 해제합니다.
     * 프레임 경계에서 호출해야 합니다.
     */
    void ProcessDeferredDestructions();

    /** 내부 SDL_GPUDevice 포인터를 반환합니다. */
    [[nodiscard]] SDL_GPUDevice* GetRawDevice() const noexcept { return raw_device; }

    /** 현재 관리 중인 텍스처 수를 반환합니다. */
    [[nodiscard]] uint32 TextureCount() const noexcept { return textures.Count(); }

    /** 현재 관리 중인 버퍼 수를 반환합니다. */
    [[nodiscard]] uint32 BufferCount() const noexcept { return buffers.Count(); }

private:
    SDL_GPUDevice* raw_device;

    SlotMap<TextureResource> textures;
    SlotMap<BufferResource> buffers;

    Array<SDL_GPUTexture*> deferred_texture_destroys;
    Array<SDL_GPUBuffer*> deferred_buffer_destroys;
};
} // namespace se::graphics
