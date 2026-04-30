#include "SimpleEngine/Graphics/Manager/PSOManager.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/ShaderUtils.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include "SDL3_shadercross/SDL_shadercross.h"

#include <ranges>


namespace se
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

    // Vertex Shader을 리플렉션 해서, 실제 사용되는 attribute만 필터링
    static const ShaderReflectionData EMPTY_REFL{};
    const FilteredVertexInputState filtered = FilterVertexInputState(
        create_info.vertex_input_state,
        shader_cache.GetReflection(create_info.vertex_shader).ValueOr(EMPTY_REFL)
    );

    const SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = vertex_shader,
        .fragment_shader = frag_shader,
        .vertex_input_state = filtered.AsState(),
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
        const SDL_GPUVertexInputState state = filtered.AsState();
        ConsoleLog(ELogLevel::Error, "Failed to create graphics pipeline!, Err: {}", SDL_GetError());
        ConsoleLog(ELogLevel::Error, "  VS: {}, PS: {}", create_info.vertex_shader, create_info.fragment_shader);
        ConsoleLog(ELogLevel::Error, "  VertexAttrs: {} -> {} (filtered)", create_info.vertex_input_state.num_vertex_attributes, state.num_vertex_attributes);
        for (uint32 i = 0; i < state.num_vertex_attributes; ++i)
        {
            ConsoleLog(ELogLevel::Error, "    [{}] loc={} fmt={} off={}", i, state.vertex_attributes[i].location, static_cast<int>(state.vertex_attributes[i].format), state.vertex_attributes[i].offset);
        }
        ConsoleLog(ELogLevel::Error, "  ColorTargets: {}, HasDepthStencil: {}", create_info.target_info.num_color_targets, create_info.target_info.has_depth_stencil_target);
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

    // 직접 SPIR-V를 읽어 Compute 파이프라인을 생성
    auto spirv_opt = ShaderCache::ReadSpvFile(create_info.compute_shader);
    if (!spirv_opt.HasValue())
    {
        return nullptr;
    }

    SDL_GPUComputePipeline* pipeline = CreateComputePipeline(*render_device, *spirv_opt, create_info.props);
    if (!pipeline)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create compute pipeline: {}", create_info.compute_shader);
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

            // 이 파이프라인이 참조하는 다른 셰이더의 역추적 맵에서도 stale 엔트리를 제거
            auto cleanup_stale_entry = [&](const VPath& other_key)
            {
                if (other_key == shader_key) { return; }
                if (const auto other_list = graphics_shader_to_pipeline_map.Find(other_key))
                {
                    if (const auto idx = other_list->Find(key))
                    {
                        other_list->RemoveAtSwap(*idx);
                    }
                }
            };

            cleanup_stale_entry(key.vertex_shader);
            cleanup_stale_entry(key.fragment_shader);
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

    // Graphics 셰이더 캐시에서 제거
    shader_cache.Invalidate(shader_key);
}

void PSOManager::ClearAll()
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

    shader_cache.ClearAll();
}
} // namespace se
