#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"

#include "SDL3/SDL_gpu.h"
#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::graphics
{
// forward declaration
class RenderDevice;

struct ShaderInputVar
{
    uint32 location;
};

struct ShaderReflectionData
{
    Array<ShaderInputVar> vertex_inputs;
};

struct GraphicsShaderCreateResult
{
    SDL_GPUShader* shader = nullptr;
    ShaderReflectionData reflection;
};

/** SPIR-V 바이트를 GPU 그래픽스 셰이더(vertex/fragment)로 생성합니다. */
[[nodiscard]] SE_CORE_API GraphicsShaderCreateResult CreateGraphicsShader(
    const RenderDevice& render_device,
    SDL_ShaderCross_ShaderStage stage,
    ArrayView<const uint8> spirv_bytecode
);

/** SPIR-V 바이트를 GPU 컴퓨트 파이프라인으로 생성합니다. */
[[nodiscard]] SE_CORE_API SDL_GPUComputePipeline* CreateComputePipeline(
    const RenderDevice& render_device,
    ArrayView<const uint8> spirv_bytecode,
    SDL_PropertiesID props = 0
);

/**
 * FilterVertexInputState의 반환값. attributes 배열을 직접 소유하므로
 * 이동 후에도 AsState()의 포인터가 항상 올바릅니다.
 */
struct SE_CORE_API FilteredVertexInputState
{
    Array<SDL_GPUVertexAttribute> attributes;
    const SDL_GPUVertexBufferDescription* vertex_buffer_descriptions = nullptr;
    uint32 num_vertex_buffers = 0;

    [[nodiscard]] SDL_GPUVertexInputState AsState() const
    {
        return {
            .vertex_buffer_descriptions = vertex_buffer_descriptions,
            .num_vertex_buffers = num_vertex_buffers,
            .vertex_attributes = attributes.Data(),
            .num_vertex_attributes = static_cast<uint32>(attributes.Len()),
        };
    }
};

/** 셰이더 리플렉션 데이터를 기반으로, 셰이더가 실제 사용하는 vertex attribute만 필터링합니다. */
[[nodiscard]] SE_CORE_API FilteredVertexInputState FilterVertexInputState(
    const SDL_GPUVertexInputState& original,
    const ShaderReflectionData& reflection
);
} // namespace se::graphics
