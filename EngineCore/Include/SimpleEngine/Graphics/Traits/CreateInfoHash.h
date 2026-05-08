#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Utility/HashUtils.h"

#include "SDL3/SDL_gpu.h"


#define SE_ARG arg
#define SE_SPECIALIZE_STD_HASH_WITHOUT_VALIDATE(type, stmt) \
template <> \
struct std::hash<type> \
{ \
    size_t operator()(const type& SE_ARG) const noexcept \
    { \
        usize seed = 0; \
        stmt \
        return static_cast<size_t>(seed); \
    } \
};

#define SE_SPECIALIZE_STD_HASH(type, validate_size, stmt) \
static_assert(sizeof(type) == validate_size, "Invalid size, please check the struct definition"); \
SE_SPECIALIZE_STD_HASH_WITHOUT_VALIDATE(type, stmt)

#define SE_HASH_COMBINE(...) se::HashUtils::Combine(seed, __VA_ARGS__);


SE_SPECIALIZE_STD_HASH(SDL_GPUVertexBufferDescription, 16,
{
    SE_HASH_COMBINE(
        SE_ARG.slot,
        SE_ARG.pitch,
        static_cast<uint32>(SE_ARG.input_rate),
        SE_ARG.instance_step_rate
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUVertexAttribute, 16,
{
    SE_HASH_COMBINE(
        SE_ARG.location,
        SE_ARG.buffer_slot,
        static_cast<uint32>(SE_ARG.format),
        SE_ARG.offset
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUVertexInputState, 32,
{
    for (uint32 i = 0; i < SE_ARG.num_vertex_buffers; ++i)
    {
        const SDL_GPUVertexBufferDescription& desc = SE_ARG.vertex_buffer_descriptions[i];
        SE_HASH_COMBINE(desc);
    }

    for (uint32 i = 0; i < SE_ARG.num_vertex_attributes; ++i)
    {
        const SDL_GPUVertexAttribute& attr = SE_ARG.vertex_attributes[i];
        SE_HASH_COMBINE(attr);
    }
})

SE_SPECIALIZE_STD_HASH(SDL_GPURasterizerState, 28,
{
    SE_HASH_COMBINE(
        static_cast<uint32>(SE_ARG.fill_mode),
        static_cast<uint32>(SE_ARG.cull_mode),
        static_cast<uint32>(SE_ARG.front_face),
        SE_ARG.depth_bias_constant_factor,
        SE_ARG.depth_bias_clamp,
        SE_ARG.depth_bias_slope_factor,
        SE_ARG.enable_depth_bias,
        SE_ARG.enable_depth_clip
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUMultisampleState, 12,
{
    SE_HASH_COMBINE(
        static_cast<uint32>(SE_ARG.sample_count),
        SE_ARG.sample_mask,
        SE_ARG.enable_mask
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUStencilOpState, 16,
{
    SE_HASH_COMBINE(
        static_cast<uint32>(SE_ARG.fail_op),
        static_cast<uint32>(SE_ARG.pass_op),
        static_cast<uint32>(SE_ARG.depth_fail_op),
        static_cast<uint32>(SE_ARG.compare_op)
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUDepthStencilState, 44,
{
    SE_HASH_COMBINE(
        static_cast<uint32>(SE_ARG.compare_op),
        SE_ARG.back_stencil_state,
        SE_ARG.front_stencil_state,
        SE_ARG.compare_mask,
        SE_ARG.write_mask,
        SE_ARG.enable_depth_test,
        SE_ARG.enable_depth_write,
        SE_ARG.enable_stencil_test
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUColorTargetBlendState, 32,
{
    SE_HASH_COMBINE(
        static_cast<uint32>(SE_ARG.src_color_blendfactor),
        static_cast<uint32>(SE_ARG.dst_color_blendfactor),
        static_cast<uint32>(SE_ARG.color_blend_op),
        static_cast<uint32>(SE_ARG.src_alpha_blendfactor),
        static_cast<uint32>(SE_ARG.dst_alpha_blendfactor),
        static_cast<uint32>(SE_ARG.alpha_blend_op),
        static_cast<uint32>(SE_ARG.color_write_mask),
        SE_ARG.enable_blend,
        SE_ARG.enable_color_write_mask
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUColorTargetDescription, 36,
{
    SE_HASH_COMBINE(
        static_cast<uint32>(SE_ARG.format),
        SE_ARG.blend_state
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUGraphicsPipelineTargetInfo, 24,
{
    for (uint32 i = 0; i < SE_ARG.num_color_targets; ++i)
    {
        const SDL_GPUColorTargetDescription& desc = SE_ARG.color_target_descriptions[i];
        SE_HASH_COMBINE(desc);
    }

    SE_HASH_COMBINE(
        SE_ARG.depth_stencil_format,
        SE_ARG.has_depth_stencil_target
    );
})

SE_SPECIALIZE_STD_HASH_WITHOUT_VALIDATE(se::GraphicsPipelineCreateInfo,
{
    SE_HASH_COMBINE(
        SE_ARG.vertex_shader,
        SE_ARG.fragment_shader,
        SE_ARG.vertex_input_state,
        static_cast<uint32>(SE_ARG.primitive_type),
        SE_ARG.rasterizer_state,
        SE_ARG.multisample_state,
        SE_ARG.depth_stencil_state,
        SE_ARG.target_info,
        SE_ARG.props
    );
})

SE_SPECIALIZE_STD_HASH_WITHOUT_VALIDATE(se::ComputePipelineCreateInfo,
{
    SE_HASH_COMBINE(
        SE_ARG.compute_shader,
        SE_ARG.props
    );
})

SE_SPECIALIZE_STD_HASH(SDL_GPUTextureCreateInfo, 36,
{
    SE_HASH_COMBINE(
        static_cast<uint32>(SE_ARG.type),
        static_cast<uint32>(SE_ARG.format),
        SE_ARG.usage,
        SE_ARG.width,
        SE_ARG.height,
        SE_ARG.layer_count_or_depth,
        SE_ARG.num_levels,
        static_cast<uint32>(SE_ARG.sample_count),
        SE_ARG.props
    )
})

SE_SPECIALIZE_STD_HASH(SDL_GPUBufferCreateInfo, 12,
{
    SE_HASH_COMBINE(
        SE_ARG.usage,
        SE_ARG.size,
        SE_ARG.props
    )
})

#undef SE_ARG
#undef SE_SPECIALIZE_STD_HASH
#undef SE_HASH_COMBINE
