#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Graphics/Memory/GpuBufferSlice.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
// forward declaration
class RenderDevice;

/**
 * GPU VRAM의 거대한 단일 할당 블록을 관리하는 클래스
 * 하나의 SDL_GPUBuffer를 소유하며, 이를 작은 Slice로 나누어 제공합니다.
 */
class SE_CORE_API GpuMemoryBlock
{
public:
    GpuMemoryBlock(RenderDevice* in_render_device, uint32 in_size, SDL_GPUBufferUsageFlags in_usage);
    ~GpuMemoryBlock();

    // 복사 방지
    GpuMemoryBlock(const GpuMemoryBlock&) = delete;
    GpuMemoryBlock& operator=(const GpuMemoryBlock&) = delete;

    // 이동만 허용
    GpuMemoryBlock(GpuMemoryBlock&& other) noexcept;
    GpuMemoryBlock& operator=(GpuMemoryBlock&& other) noexcept;

public:
    /** 블록 내에서 메모리를 할당합니다. (Linear Allocation) */
    [[nodiscard]] bool AllocateSlice(uint32 in_size, uint32 in_alignment, GpuBufferSlice& out_slice);

    [[nodiscard]] uint32 GetTotalSize() const { return total_size; }
    [[nodiscard]] uint32 GetUsedSize() const { return used_offset; }
    [[nodiscard]] SDL_GPUBufferUsageFlags GetUsage() const { return usage_flags; }
    [[nodiscard]] SDL_GPUBuffer* GetNativeBuffer() const { return buffer; }

private:
    RenderDevice* render_device = nullptr;
    SDL_GPUBuffer* buffer = nullptr; // GPU Buffer 리소스

    SDL_GPUBufferUsageFlags usage_flags = 0; // Buffer의 사용 용도
    uint32 total_size = 0;                   // 버퍼 크기
    uint32 used_offset = 0;                  // 버퍼를 할당할 수 있는 시작 오프셋
};
} // namespace se
