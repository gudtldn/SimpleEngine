#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Rendering/Memory/GpuBufferSlice.h"

#include "SDL3/SDL_gpu.h"


namespace se::rendering
{
/**
 * GPU VRAM의 거대한 단일 할당 블록을 관리하는 클래스
 * 하나의 SDL_GPUBuffer를 소유하며, 이를 작은 Slice로 나누어 제공합니다.
 */
class SE_CORE_API GpuMemoryBlock
{
public:
    GpuMemoryBlock(SDL_GPUDevice* in_device, uint32 in_size, SDL_GPUBufferUsageFlags in_usage);
    ~GpuMemoryBlock();

    // 이동만 허용
    GpuMemoryBlock(const GpuMemoryBlock&) = delete;
    GpuMemoryBlock& operator=(const GpuMemoryBlock&) = delete;
    GpuMemoryBlock(GpuMemoryBlock&& other) noexcept;
    GpuMemoryBlock& operator=(GpuMemoryBlock&& other) noexcept;

public:
    [[nodiscard]] bool AllocateSlice(uint32 in_size, uint32 in_alignment, GpuBufferSlice& out_slice);
    [[nodiscard]] SDL_GPUBuffer* GetNativeBuffer() const noexcept { return buffer; }

private:
    SDL_GPUDevice* device;
    SDL_GPUBuffer* buffer; // GPU Buffer 리소스

    uint32 total_size;  // 버퍼 크기
    uint32 used_offset; // 버퍼를 할당할 수 있는 시작 오프셋
};
}  // namespace se::rendering
