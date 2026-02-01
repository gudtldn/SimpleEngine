#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
/**
 * GPU에 로드된 텍스처 리소스의 핸들 및 메타데이터
 */
struct GpuTexture
{
    SDL_GPUTexture* texture = nullptr;
    uint32 width = 0;
    uint32 height = 0;
    SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_INVALID;

    [[nodiscard]] bool IsValid() const noexcept { return texture != nullptr; }
    void Reset() { *this = GpuTexture{}; }
};
}  // namespace se::graphics
