#pragma once

#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoEquals.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
/**
 * Hashing 가능한 SDL_GPUGraphicsPipelineCreateInfo 구조체
 */
struct GraphicsPipelineCreateInfo
{
    VPath shader_program; // 정점·픽셀 스테이지를 모두 가진 셰이더 소스 (예: "CoreShader://Default.hlsl")

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
struct ComputePipelineCreateInfo
{
    VPath compute_program; // 컴퓨트 스테이지를 가진 셰이더 소스

    SDL_PropertiesID props = 0;

    bool operator==(const ComputePipelineCreateInfo& other) const = default;
};
} // namespace se
