#pragma once

#include "SimpleEngine/Core/Container/Array.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
struct ShaderInputVar
{
    u32 location;
};

struct ShaderReflectionData
{
    Array<ShaderInputVar> vertex_inputs;
};

/**
 * FilterVertexInputState의 반환값. attributes 배열을 직접 소유하므로
 * 이동 후에도 AsState()의 포인터가 항상 올바릅니다.
 */
struct SE_CORE_API FilteredVertexInputState
{
    Array<SDL_GPUVertexAttribute> attributes;
    const SDL_GPUVertexBufferDescription* vertex_buffer_descriptions = nullptr;
    u32 num_vertex_buffers = 0;

    [[nodiscard]] SDL_GPUVertexInputState AsState() const
    {
        return {
            .vertex_buffer_descriptions = vertex_buffer_descriptions,
            .num_vertex_buffers = num_vertex_buffers,
            .vertex_attributes = attributes.Data(),
            .num_vertex_attributes = static_cast<u32>(attributes.Len()),
        };
    }
};

/** 셰이더 리플렉션 데이터를 기반으로, 셰이더가 실제 사용하는 vertex attribute만 필터링합니다. */
[[nodiscard]] SE_CORE_API FilteredVertexInputState FilterVertexInputState(
    const SDL_GPUVertexInputState& original,
    const ShaderReflectionData& reflection
);
} // namespace se
