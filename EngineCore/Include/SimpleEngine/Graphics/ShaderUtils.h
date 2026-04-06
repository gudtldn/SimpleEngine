#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"

#include "SDL3/SDL_gpu.h"
#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::graphics
{
// forward declaration
class RenderDevice;

/** SPIR-V 바이트를 GPU 그래픽스 셰이더(vertex/fragment)로 생성합니다. */
[[nodiscard]] SE_CORE_API SDL_GPUShader* CreateGraphicsShader(
    const RenderDevice& render_device,
    SDL_ShaderCross_ShaderStage stage,
    ArrayView<const uint8> spirv_bytecode
);

/** SPIR-V 바이트를 GPU 컴퓨트 파이프라인으로 생성합니다. */
[[nodiscard]] SE_CORE_API SDL_GPUComputePipeline* CreateComputePipeline(
    const RenderDevice& render_device,
    ArrayView<const uint8> spirv_bytecode
);
} // namespace se::graphics
