#include "SimpleEngine/Graphics/Manager/PSOManager.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include <ranges>


namespace se::graphics
{
PSOManager::PSOManager(RenderDevice& in_render_device)
    : render_device(&in_render_device)
    , shader_cache(in_render_device)
{
}

PSOManager::~PSOManager()
{
    for (SDL_GPUGraphicsPipeline* pipeline : cached_graphics_pipelines | std::views::values)
    {
        SDL_ReleaseGPUGraphicsPipeline(render_device->GetRawDevice(), pipeline);
    }
    cached_graphics_pipelines.Clear();

    for (SDL_GPUComputePipeline* pipeline : cached_compute_pipelines | std::views::values)
    {
        SDL_ReleaseGPUComputePipeline(render_device->GetRawDevice(), pipeline);
    }
    cached_compute_pipelines.Clear();
}

SDL_GPUGraphicsPipeline* PSOManager::GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info)
{
    if (Optional pipeline_opt = cached_graphics_pipelines.Find(create_info))
    {
        return *pipeline_opt;
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

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(render_device->GetRawDevice(), &info);
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
    return SDL_CreateGPUComputePipeline(render_device->GetRawDevice(), &create_info.compute_pipeline_create_info);
}

void PSOManager::EndFrame()
{
    shader_cache.ClearCache();
}
}  // namespace se::graphics
