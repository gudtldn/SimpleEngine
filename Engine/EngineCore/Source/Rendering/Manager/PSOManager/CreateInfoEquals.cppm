export module SE.Rendering:Manager.PSOManager.CreateInfoEquals;

import SE.Types;
import SE.Utility;
import std;

import "SDL3/SDL_gpu.h";


#define SE_DEFINE_EQUALS_OPERATOR(type) \
    bool operator==(const type& a, const type& b) noexcept

#define SE_PROPERTY_EQUAL(prop) \
    a.prop == b.prop

#define SE_IF_NOT_EQUAL_RETURN(prop) \
    if (a.prop != b.prop) { return false; }


inline namespace graphics_pipeline_create_info_equals
{
SE_DEFINE_EQUALS_OPERATOR(SDL_GPUVertexBufferDescription)
{
    return SE_PROPERTY_EQUAL(slot)
        && SE_PROPERTY_EQUAL(pitch)
        && SE_PROPERTY_EQUAL(input_rate)
        && SE_PROPERTY_EQUAL(instance_step_rate);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUVertexAttribute)
{
    return SE_PROPERTY_EQUAL(location)
        && SE_PROPERTY_EQUAL(buffer_slot)
        && SE_PROPERTY_EQUAL(format)
        && SE_PROPERTY_EQUAL(offset);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUVertexInputState)
{
    SE_IF_NOT_EQUAL_RETURN(num_vertex_buffers);
    for (uint32 i = 0; i < a.num_vertex_buffers; ++i)
    {
        SE_IF_NOT_EQUAL_RETURN(vertex_buffer_descriptions[i]);
    }

    SE_IF_NOT_EQUAL_RETURN(num_vertex_attributes);
    for (uint32 i = 0; i < a.num_vertex_attributes; ++i)
    {
        SE_IF_NOT_EQUAL_RETURN(vertex_attributes[i]);
    }

    return true;
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPURasterizerState)
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

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUMultisampleState)
{
    return SE_PROPERTY_EQUAL(sample_count)
        && SE_PROPERTY_EQUAL(sample_mask)
        && SE_PROPERTY_EQUAL(enable_mask);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUStencilOpState)
{
    return SE_PROPERTY_EQUAL(fail_op)
        && SE_PROPERTY_EQUAL(pass_op)
        && SE_PROPERTY_EQUAL(depth_fail_op)
        && SE_PROPERTY_EQUAL(compare_op);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUDepthStencilState)
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

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUColorTargetBlendState)
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

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUColorTargetDescription)
{
    return SE_PROPERTY_EQUAL(format)
        && SE_PROPERTY_EQUAL(blend_state);
}

SE_DEFINE_EQUALS_OPERATOR(SDL_GPUGraphicsPipelineTargetInfo)
{
    SE_IF_NOT_EQUAL_RETURN(num_color_targets);
    for (uint32 i = 0; i < a.num_color_targets; ++i)
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
