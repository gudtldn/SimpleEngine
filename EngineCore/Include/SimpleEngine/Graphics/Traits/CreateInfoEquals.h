#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "SDL3/SDL_gpu.h"


#define SE_DEFINE_EQUALS_OPERATOR(type, validate_size) \
    static_assert(sizeof(type) == validate_size, "Invalid size, please check the struct definition"); \
    inline bool operator==(const type& a, const type& b) noexcept

#define SE_PROPERTY_EQUAL(prop) \
    a.prop == b.prop

#define SE_IF_NOT_EQUAL_RETURN(prop) \
    if (a.prop != b.prop) { return false; }


inline namespace graphics_pipeline_create_info_equals
{
SE_DEFINE_EQUALS_OPERATOR(SDL_GPUVertexBufferDescription, 16)
{
    return SE_PROPERTY_EQUAL(slot)
        && SE_PROPERTY_EQUAL(pitch)
        && SE_PROPERTY_EQUAL(input_rate)
        && SE_PROPERTY_EQUAL(instance_step_rate);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUVertexAttribute, 16)
{
    return SE_PROPERTY_EQUAL(location)
        && SE_PROPERTY_EQUAL(buffer_slot)
        && SE_PROPERTY_EQUAL(format)
        && SE_PROPERTY_EQUAL(offset);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUVertexInputState, 32)
{
    SE_IF_NOT_EQUAL_RETURN(num_vertex_buffers);
    for (u32 i = 0; i < a.num_vertex_buffers; ++i)
    {
        SE_IF_NOT_EQUAL_RETURN(vertex_buffer_descriptions[i]);
    }

    SE_IF_NOT_EQUAL_RETURN(num_vertex_attributes);
    for (u32 i = 0; i < a.num_vertex_attributes; ++i)
    {
        SE_IF_NOT_EQUAL_RETURN(vertex_attributes[i]);
    }

    return true;
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPURasterizerState, 28)
{
    return SE_PROPERTY_EQUAL(fill_mode)
        && SE_PROPERTY_EQUAL(cull_mode)
        && SE_PROPERTY_EQUAL(front_face)
        && SE_PROPERTY_EQUAL(depth_bias_constant_factor)
        && SE_PROPERTY_EQUAL(depth_bias_clamp)
        && SE_PROPERTY_EQUAL(depth_bias_slope_factor)
        && SE_PROPERTY_EQUAL(enable_depth_bias)
        && SE_PROPERTY_EQUAL(enable_depth_clip);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUMultisampleState, 12)
{
    return SE_PROPERTY_EQUAL(sample_count)
        && SE_PROPERTY_EQUAL(sample_mask)
        && SE_PROPERTY_EQUAL(enable_mask);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUStencilOpState, 16)
{
    return SE_PROPERTY_EQUAL(fail_op)
        && SE_PROPERTY_EQUAL(pass_op)
        && SE_PROPERTY_EQUAL(depth_fail_op)
        && SE_PROPERTY_EQUAL(compare_op);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUDepthStencilState, 44)
{
    return SE_PROPERTY_EQUAL(compare_op)
        && SE_PROPERTY_EQUAL(back_stencil_state)
        && SE_PROPERTY_EQUAL(front_stencil_state)
        && SE_PROPERTY_EQUAL(compare_mask)
        && SE_PROPERTY_EQUAL(write_mask)
        && SE_PROPERTY_EQUAL(enable_depth_test)
        && SE_PROPERTY_EQUAL(enable_depth_write)
        && SE_PROPERTY_EQUAL(enable_stencil_test);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUColorTargetBlendState, 32)
{
    return SE_PROPERTY_EQUAL(src_color_blendfactor)
        && SE_PROPERTY_EQUAL(dst_color_blendfactor)
        && SE_PROPERTY_EQUAL(color_blend_op)
        && SE_PROPERTY_EQUAL(src_alpha_blendfactor)
        && SE_PROPERTY_EQUAL(dst_alpha_blendfactor)
        && SE_PROPERTY_EQUAL(alpha_blend_op)
        && SE_PROPERTY_EQUAL(color_write_mask)
        && SE_PROPERTY_EQUAL(enable_blend)
        && SE_PROPERTY_EQUAL(enable_color_write_mask);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUColorTargetDescription, 36)
{
    return SE_PROPERTY_EQUAL(format)
        && SE_PROPERTY_EQUAL(blend_state);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUGraphicsPipelineTargetInfo, 24)
{
    SE_IF_NOT_EQUAL_RETURN(num_color_targets);
    for (u32 i = 0; i < a.num_color_targets; ++i)
    {
        SE_IF_NOT_EQUAL_RETURN(color_target_descriptions[i]);
    }

    return SE_PROPERTY_EQUAL(depth_stencil_format)
        && SE_PROPERTY_EQUAL(has_depth_stencil_target);
}
}


inline namespace compute_pipeline_create_info
{
}

inline namespace other_create_info
{
SE_DEFINE_EQUALS_OPERATOR(SDL_GPUTextureCreateInfo, 36)
{
    return SE_PROPERTY_EQUAL(type)
        && SE_PROPERTY_EQUAL(format)
        && SE_PROPERTY_EQUAL(usage)
        && SE_PROPERTY_EQUAL(width)
        && SE_PROPERTY_EQUAL(height)
        && SE_PROPERTY_EQUAL(layer_count_or_depth)
        && SE_PROPERTY_EQUAL(num_levels)
        && SE_PROPERTY_EQUAL(sample_count)
        && SE_PROPERTY_EQUAL(props);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUBufferCreateInfo, 12)
{
    return SE_PROPERTY_EQUAL(size)
        && SE_PROPERTY_EQUAL(usage)
        && SE_PROPERTY_EQUAL(props);
}
}

#undef SE_DEFINE_EQUALS_OPERATOR
#undef SE_PROPERTY_EQUAL
#undef SE_IF_NOT_EQUAL_RETURN
