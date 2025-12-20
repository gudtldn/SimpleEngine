#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SDL3/SDL_gpu.h"


namespace se::rendering
{
/**
  * GPU 버퍼의 특정 구간(Slice)을 나타내는 메타데이터
  * 물리적인 버퍼 핸들과 해당 버퍼 내의 오프셋/크기 정보를 가집니다
 */
struct GpuBufferSlice
{
    // GpuMemoryBlock에서 가져온 GPU Buffer 리소스
    SDL_GPUBuffer* buffer = nullptr;

    uint32 offset = 0;       // 버퍼 시작점으로부터의 바이트 오프셋
    uint32 size = 0;         // 할당된 크기 (bytes)
    uint32 index_offset = 0; // (Mesh의 경우) Vertex Data 끝, Index Data 시작점
    uint32 index_count = 0;  // Index Data의 개수

    [[nodiscard]] bool IsValid() const noexcept { return buffer != nullptr && size > 0; }
    void Reset() { *this = GpuBufferSlice{}; }

    explicit operator bool() const noexcept { return IsValid(); }
};
} // namespace se::rendering
