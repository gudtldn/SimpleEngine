#include "Rendering/Manager/PSOManager.h"

#include <ranges>
#include "Core/Logging/Logging.h"


namespace se::rendering
{
PSOManager::PSOManager(SDL_GPUDevice* in_device)
    : device(in_device)
    , shader_cache(in_device)
{
}

PSOManager::~PSOManager()
{
    for (SDL_GPUGraphicsPipeline* pipeline : cached_graphics_pipelines | std::views::values)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    }
    cached_graphics_pipelines.clear();

    for (SDL_GPUComputePipeline* pipeline : cached_compute_pipelines | std::views::values)
    {
        SDL_ReleaseGPUComputePipeline(device, pipeline);
    }
    cached_compute_pipelines.clear();
}

SDL_GPUGraphicsPipeline* PSOManager::GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info)
{
    if (cached_graphics_pipelines.contains(create_info))
    {
        return cached_graphics_pipelines[create_info];
    }

    SDL_GPUShader* vertex_shader = shader_cache.GetOrCreate(create_info.vertex_shader_request);
    if (!vertex_shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get vertex shader from cache!");
        return nullptr;
    }

    SDL_GPUShader* frag_shader = shader_cache.GetOrCreate(create_info.fragment_shader_request);
    if (!frag_shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get fragment shader from cache!");
        return nullptr;
    }

    const SDL_GPUGraphicsPipelineCreateInfo info{
        .vertex_shader = vertex_shader,
        .fragment_shader = frag_shader,
        .vertex_input_state = create_info.vertex_input_state,
        .primitive_type = create_info.primitive_type,
        .rasterizer_state = create_info.rasterizer_state,
        .multisample_state = create_info.multisample_state,
        .depth_stencil_state = create_info.depth_stencil_state,
        .target_info = create_info.target_info,
        .props = create_info.props
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!pipeline)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create graphics pipeline!, Err: {}", SDL_GetError());
        return nullptr;
    }

    cached_graphics_pipelines[create_info] = pipeline;
    return pipeline;
}

SDL_GPUComputePipeline* PSOManager::GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info)
{
    return SDL_CreateGPUComputePipeline(device, &create_info.compute_pipeline_create_info);
}

void PSOManager::EndFrame()
{
    shader_cache.ClearCache();
}
}
