export module SE.Rendering:Manager.PSOManager.CreateInfoHash;
import :Manager.PSOManager.PipelineCreateInfo;
import :ShaderProvider.IShaderProvider;

import SE.Types;
import SE.Utility;
import std;

import "SDL3/SDL_gpu.h";

#define SE_SPECIALIZE_STD_HASH(type, var_name, stmt) \
template <> \
struct std::hash<type> \
{ \
    size_t operator()(const type& var_name) const noexcept \
    { \
        size_t seed = 0; \
        stmt \
        return seed; \
    } \
};

using se::utility::hash::HashCombine;


SE_SPECIALIZE_STD_HASH(SDL_GPUVertexBufferDescription, desc,
{
    HashCombine(
        seed,
        desc.slot,
        desc.pitch,
        static_cast<uint32>(desc.input_rate),
        desc.instance_step_rate
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUVertexAttribute, attr,
{
    HashCombine(
        seed,
        attr.location,
        attr.buffer_slot,
        static_cast<uint32>(attr.format),
        attr.offset
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUVertexInputState, state,
{
    for (uint32 i = 0; i < state.num_vertex_buffers; ++i)
    {
        const SDL_GPUVertexBufferDescription& desc = state.vertex_buffer_descriptions[i];
        HashCombine(seed, desc);
    }

    for (uint32 i = 0; i < state.num_vertex_attributes; ++i)
    {
        const SDL_GPUVertexAttribute& attr = state.vertex_attributes[i];
        HashCombine(seed, attr);
    }
})

SE_SPECIALIZE_STD_HASH(SDL_GPURasterizerState, state,
{
    HashCombine(
        seed,
        static_cast<uint32>(state.fill_mode),
        static_cast<uint32>(state.cull_mode),
        static_cast<uint32>(state.front_face),
        state.depth_bias_constant_factor,
        state.depth_bias_clamp,
        state.depth_bias_slope_factor,
        state.enable_depth_bias,
        state.enable_depth_clip
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUMultisampleState, state,
{
    HashCombine(
        seed,
        static_cast<uint32>(state.sample_count),
        state.sample_mask,
        state.enable_mask
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUStencilOpState, state,
{
    HashCombine(
        seed,
        static_cast<uint32>(state.fail_op),
        static_cast<uint32>(state.pass_op),
        static_cast<uint32>(state.depth_fail_op),
        static_cast<uint32>(state.compare_op)
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUDepthStencilState, state,
{
    HashCombine(
        seed,
        static_cast<uint32>(state.compare_op),
        state.back_stencil_state,
        state.front_stencil_state,
        state.compare_mask,
        state.write_mask,
        state.enable_depth_test,
        state.enable_depth_write,
        state.enable_stencil_test
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUColorTargetBlendState, state,
{
    HashCombine(
        seed,
        static_cast<uint32>(state.src_color_blendfactor),
        static_cast<uint32>(state.dst_color_blendfactor),
        static_cast<uint32>(state.color_blend_op),
        static_cast<uint32>(state.src_alpha_blendfactor),
        static_cast<uint32>(state.dst_alpha_blendfactor),
        static_cast<uint32>(state.alpha_blend_op),
        static_cast<uint32>(state.color_write_mask),
        state.enable_blend,
        state.enable_color_write_mask
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUColorTargetDescription, desc,
{
    HashCombine(
        seed,
        static_cast<uint32>(desc.format),
        desc.blend_state
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUGraphicsPipelineTargetInfo, info,
{
    for (uint32 i = 0; i < info.num_color_targets; ++i)
    {
        const SDL_GPUColorTargetDescription& desc = info.color_target_descriptions[i];
        HashCombine(seed, desc);
    }

    HashCombine(
        seed,
        info.depth_stencil_format,
        info.has_depth_stencil_target
    );
})

SE_SPECIALIZE_STD_HASH(GraphicsPipelineCreateInfo, info,
{
    HashCombine(
        seed,
        info.vertex_shader_request,
        info.fragment_shader_request,
        info.vertex_input_state,
        static_cast<uint32>(info.primitive_type),
        info.rasterizer_state,
        info.multisample_state,
        info.depth_stencil_state,
        info.target_info,
        info.props
    );
})

SE_SPECIALIZE_STD_HASH(ComputePipelineCreateInfo, info,
{
    HashCombine(
        seed,
        info.placeholder
    );
})
