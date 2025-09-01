export module SE.Rendering:Manager.PSOManager.PipelineCreateInfo;
import :Manager.PSOManager.PipelineCreateInfoEquals;
import :ShaderProvider.IShaderProvider;

import SE.Types;
import std;

import "SDL3/SDL_gpu.h";

using namespace se::rendering::shader_provider;


/**
 * Hashing 가능한 SDL_GPUGraphicsPipelineCreateInfo 구조체
 */
export struct GraphicsPipelineCreateInfo
{
    ShaderRequest vertex_shader_request;
    ShaderRequest fragment_shader_request;

    SDL_GPUVertexInputState vertex_input_state;    // The vertex layout of the graphics pipeline.
    SDL_GPUPrimitiveType primitive_type;           // The primitive topology of the graphics pipeline.
    SDL_GPURasterizerState rasterizer_state;       // The rasterizer state of the graphics pipeline.
    SDL_GPUMultisampleState multisample_state;     // The multisample state of the graphics pipeline.
    SDL_GPUDepthStencilState depth_stencil_state;  // The depth-stencil state of the graphics pipeline.
    SDL_GPUGraphicsPipelineTargetInfo target_info; // Formats and blend modes for the render targets of the graphics pipeline.

    SDL_PropertiesID props; // A properties ID for extensions. Should be 0 if no extensions are needed.

    bool operator==(const GraphicsPipelineCreateInfo& other) const = default;
};


/**
 * Hashing 가능한 SDL_GPUComputePipelineCreateInfo 구조체
 */
export struct ComputePipelineCreateInfo
{
    uint32 placeholder;
    SDL_GPUComputePipelineCreateInfo compute_pipeline_create_info;

    // TODO: Implement

    bool operator==(const ComputePipelineCreateInfo& other) const
    {
        return placeholder == other.placeholder;
    }
};
