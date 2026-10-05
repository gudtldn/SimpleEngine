#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Shader/ShaderInterface.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::editor
{
/** 스테이지가 사용하는 descriptor set 번호 */
struct StageSets
{
    u32 resources = 0;
    Optional<u32> readwrite_resources;
    u32 uniform_buffers = 0;
};

/**
 * SDL3 GPU 규약의 스테이지별 set (SPIR-V descriptor set = HLSL register space)
 * https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
 * https://wiki.libsdl.org/SDL3/SDL_CreateGPUComputePipeline#remarks
 */
[[nodiscard]] inline StageSets SetsOf(EShaderStage stage)
{
    switch (stage)
    {
    case EShaderStage::Vertex:   return { .resources = 0, .uniform_buffers = 1 };
    case EShaderStage::Fragment: return { .resources = 2, .uniform_buffers = 3 };
    case EShaderStage::Compute:  return { .resources = 0, .readwrite_resources = 1u, .uniform_buffers = 2 };
    }
    SE_UNREACHABLE();
}
} // namespace se::editor
