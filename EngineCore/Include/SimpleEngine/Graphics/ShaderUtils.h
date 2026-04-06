#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/Optional.h"

#include "SDL3/SDL_gpu.h"
#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::graphics
{
// forward declaration
class RenderDevice;

/** 문자열(파일명 등)에서 ShaderStage를 자동으로 탐지합니다. (e.g. "DebugLine.vert" -> VERTEX) */
[[nodiscard, deprecated]]
SE_CORE_API Optional<SDL_ShaderCross_ShaderStage> DetermineShaderStage(StringView name_hint);

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
