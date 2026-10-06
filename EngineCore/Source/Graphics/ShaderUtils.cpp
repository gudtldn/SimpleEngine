#include "SimpleEngine/Graphics/ShaderUtils.h"


namespace se
{
FilteredVertexInputState FilterVertexInputState(
    const SDL_GPUVertexInputState& original,
    const ShaderReflectionData& reflection
)
{
    FilteredVertexInputState result;
    result.vertex_buffer_descriptions = original.vertex_buffer_descriptions;
    result.num_vertex_buffers = original.num_vertex_buffers;
    result.attributes.Reserve(original.num_vertex_attributes);

    if (reflection.vertex_inputs.IsEmpty())
    {
        // 필터링 없음 - 원본 전체 복사
        for (u32 i = 0; i < original.num_vertex_attributes; ++i)
        {
            result.attributes.Push(original.vertex_attributes[i]);
        }
        return result;
    }

    for (u32 i = 0; i < original.num_vertex_attributes; ++i)
    {
        const SDL_GPUVertexAttribute& attr = original.vertex_attributes[i];
        for (const ShaderInputVar& input : reflection.vertex_inputs)
        {
            if (attr.location == input.location)
            {
                result.attributes.Push(attr);
                break;
            }
        }
    }

    return result;
}
} // namespace se
