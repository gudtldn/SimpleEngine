#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "SDL3/SDL_gpu.h"

#include <algorithm>


namespace se
{
/**
 * 특정 밉 레벨의 GPU 텍스처 크기(바이트)를 계산합니다.
 * @param format SDL GPU 텍스처 포맷
 * @param width 밉 레벨의 너비 (픽셀)
 * @param height 밉 레벨의 높이 (픽셀)
 * @param depth_or_layer_count 3D 텍스처의 depth 또는 그 외의 layer count
 */
[[nodiscard]] inline u64 CalculateMipLevelSize(
    SDL_GPUTextureFormat format,
    u32 width,
    u32 height,
    u32 depth_or_layer_count = 1
)
{
    return SDL_CalculateGPUTextureFormatSize(format, width, height, depth_or_layer_count);
}

/**
 * 텍스처 전체 메모리 크기(밉 체인 합산, 바이트)를 계산합니다.
 * @param format SDL GPU 텍스처 포맷
 * @param width 밉 0의 너비 (픽셀)
 * @param height 밉 0의 높이 (픽셀)
 * @param num_levels 누적할 밉 레벨 개수
 * @param depth_or_layer_count 3D 텍스처의 depth 또는 그 외의 layer count
 */
[[nodiscard]] inline u64 CalculateTextureMemory(
    SDL_GPUTextureFormat format,
    u32 width,
    u32 height,
    u32 num_levels,
    u32 depth_or_layer_count = 1
)
{
    u64 total = 0;
    u32 w = width;
    u32 h = height;
    for (u32 level = 0; level < num_levels; ++level)
    {
        total += CalculateMipLevelSize(format, w, h, depth_or_layer_count);
        if (w == 1 && h == 1)
        {
            break;
        }
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }
    return total;
}

/** SDL_GPUTextureCreateInfo로부터 텍스처 전체 메모리 크기를 계산합니다. */
[[nodiscard]] inline u64 CalculateTextureMemoryFromCreateInfo(const SDL_GPUTextureCreateInfo& desc)
{
    return CalculateTextureMemory(
        desc.format,
        desc.width,
        desc.height,
        desc.num_levels == 0 ? 1 : desc.num_levels,
        desc.layer_count_or_depth == 0 ? 1 : desc.layer_count_or_depth
    );
}
} // namespace se
