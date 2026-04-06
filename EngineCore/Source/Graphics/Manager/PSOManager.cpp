#include "SimpleEngine/Graphics/Manager/PSOManager.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include "SDL3_shadercross/SDL_shadercross.h"

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
    graphics_shader_to_pipeline_map.Clear();

    for (SDL_GPUComputePipeline* pipeline : cached_compute_pipelines | std::views::values)
    {
        SDL_ReleaseGPUComputePipeline(render_device->GetRawDevice(), pipeline);
    }
    cached_compute_pipelines.Clear();
    compute_shader_to_pipeline_map.Clear();
}

SDL_GPUGraphicsPipeline* PSOManager::GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info)
{
    if (const auto pipeline = cached_graphics_pipelines.Find(create_info))
    {
        return *pipeline;
    }

    SDL_GPUShader* vertex_shader = shader_cache.GetOrCreateShader(create_info.vertex_shader, SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    if (!vertex_shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get vertex shader from cache!");
        return nullptr;
    }

    SDL_GPUShader* frag_shader = shader_cache.GetOrCreateShader(create_info.fragment_shader, SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    if (!frag_shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get fragment shader from cache!");
        return nullptr;
    }

    const SDL_GPUGraphicsPipelineCreateInfo info = {
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

    // 역추적 인덱스 등록
    graphics_shader_to_pipeline_map[create_info.vertex_shader].Push(create_info);
    graphics_shader_to_pipeline_map[create_info.fragment_shader].Push(create_info);

    return pipeline;
}

SDL_GPUComputePipeline* PSOManager::GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info)
{
    if (const auto pipeline = cached_compute_pipelines.Find(create_info))
    {
        return *pipeline;
    }

    SDL_GPUComputePipeline* pipeline = shader_cache.GetOrCreateComputePipeline(create_info.compute_shader);
    if (!pipeline)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get compute pipeline from cache!");
        return nullptr;
    }

    cached_compute_pipelines[create_info] = pipeline;

    // 역추적 인덱스 등록
    compute_shader_to_pipeline_map[create_info.compute_shader].Push(create_info);

    return pipeline;
}

void PSOManager::InvalidateShader(const VPath& shader_key)
{
    // Graphics 파이프라인 무효화
    if (const auto pipelines = graphics_shader_to_pipeline_map.Find(shader_key))
    {
        for (const GraphicsPipelineCreateInfo& key : *pipelines)
        {
            if (const auto pipeline = cached_graphics_pipelines.Find(key))
            {
                SDL_ReleaseGPUGraphicsPipeline(render_device->GetRawDevice(), *pipeline);
                cached_graphics_pipelines.Remove(key);
            }
        }
        graphics_shader_to_pipeline_map.Remove(shader_key);
    }

    // Compute 파이프라인 무효화
    if (const auto pipelines = compute_shader_to_pipeline_map.Find(shader_key))
    {
        for (const ComputePipelineCreateInfo& key : *pipelines)
        {
            if (const auto pipeline = cached_compute_pipelines.Find(key))
            {
                SDL_ReleaseGPUComputePipeline(render_device->GetRawDevice(), *pipeline);
                cached_compute_pipelines.Remove(key);
            }
        }
        compute_shader_to_pipeline_map.Remove(shader_key);
    }

    // 셰이더 캐시에서 제거
    shader_cache.Invalidate(shader_key);
}
} // namespace se::graphics
