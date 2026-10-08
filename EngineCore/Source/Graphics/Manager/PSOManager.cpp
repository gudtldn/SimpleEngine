#include "SimpleEngine/Graphics/Manager/PSOManager.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/ShaderUtils.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include <ranges>


namespace se
{
namespace
{
/** 번들에 기록된 정점 셰이더 입력 location을 정점 속성 필터 입력으로 바꿉니다. */
[[nodiscard]] ShaderReflectionData VertexReflectionOf(Optional<const ShaderProgramInterface&> program)
{
    ShaderReflectionData reflection;
    if (!program)
    {
        return reflection;
    }

    if (const auto vertex = program->FindStage(EShaderStage::Vertex))
    {
        for (const ShaderVertexInput& input : vertex->vertex_inputs)
        {
            reflection.vertex_inputs.Push({ .location = input.location });
        }
    }
    return reflection;
}
} // namespace


PSOManager::PSOManager(RenderDevice& in_render_device, const IShaderBundleSource& shader_bundle_source)
    : render_device(&in_render_device)
    , shader_library(in_render_device.GetRawDevice(), shader_bundle_source)
{
}

PSOManager::~PSOManager()
{
    for (SDL_GPUGraphicsPipeline* pipeline : cached_graphics_pipelines | std::views::values)
    {
        SDL_ReleaseGPUGraphicsPipeline(render_device->GetRawDevice(), pipeline);
    }
    cached_graphics_pipelines.Clear();
    graphics_program_to_pipeline_map.Clear();

    for (SDL_GPUComputePipeline* pipeline : cached_compute_pipelines | std::views::values)
    {
        SDL_ReleaseGPUComputePipeline(render_device->GetRawDevice(), pipeline);
    }
    cached_compute_pipelines.Clear();
    compute_program_to_pipeline_map.Clear();
}

SDL_GPUGraphicsPipeline* PSOManager::GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info)
{
    if (const auto pipeline = cached_graphics_pipelines.Find(create_info))
    {
        return *pipeline;
    }

    SDL_GPUShader* vertex_shader = shader_library.GetOrCreateShader(create_info.shader_program, EShaderStage::Vertex);
    if (!vertex_shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get vertex shader: {}", create_info.shader_program);
        return nullptr;
    }

    SDL_GPUShader* frag_shader = shader_library.GetOrCreateShader(create_info.shader_program, EShaderStage::Fragment);
    if (!frag_shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get fragment shader: {}", create_info.shader_program);
        return nullptr;
    }

    // 정점 셰이더가 실제로 쓰는 attribute만 필터링
    const FilteredVertexInputState filtered = FilterVertexInputState(
        create_info.vertex_input_state,
        VertexReflectionOf(shader_library.FindInterface(create_info.shader_program))
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
        .props = create_info.props,
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(render_device->GetRawDevice(), &info);
    if (!pipeline)
    {
        const SDL_GPUVertexInputState state = filtered.AsState();
        ConsoleLog(ELogLevel::Error, "Failed to create graphics pipeline!, Err: {}", SDL_GetError());
        ConsoleLog(ELogLevel::Error, "  Program: {}", create_info.shader_program);
        ConsoleLog(ELogLevel::Error, "  VertexAttrs: {} -> {} (filtered)", create_info.vertex_input_state.num_vertex_attributes, state.num_vertex_attributes);
        for (u32 i = 0; i < state.num_vertex_attributes; ++i)
        {
            ConsoleLog(ELogLevel::Error, "    [{}] loc={} fmt={} off={}", i, state.vertex_attributes[i].location, static_cast<int>(state.vertex_attributes[i].format), state.vertex_attributes[i].offset);
        }
        ConsoleLog(ELogLevel::Error, "  ColorTargets: {}, HasDepthStencil: {}", create_info.target_info.num_color_targets, create_info.target_info.has_depth_stencil_target);
        return nullptr;
    }

    cached_graphics_pipelines.Insert(create_info, pipeline);

    // 역추적 인덱스 등록
    graphics_program_to_pipeline_map.Entry(create_info.shader_program).OrDefault().Push(create_info);

    return pipeline;
}

SDL_GPUComputePipeline* PSOManager::GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info)
{
    if (const auto pipeline = cached_compute_pipelines.Find(create_info))
    {
        return *pipeline;
    }

    SDL_GPUComputePipeline* pipeline = shader_library.CreateComputePipeline(create_info.compute_program, create_info.props);
    if (!pipeline)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create compute pipeline: {}", create_info.compute_program);
        return nullptr;
    }

    cached_compute_pipelines.Insert(create_info, pipeline);

    // 역추적 인덱스 등록
    compute_program_to_pipeline_map.Entry(create_info.compute_program).OrDefault().Push(create_info);

    return pipeline;
}

void PSOManager::InvalidateShader(const VPath& shader_program)
{
    // Graphics 파이프라인 무효화
    if (const auto pipelines = graphics_program_to_pipeline_map.Find(shader_program))
    {
        for (const GraphicsPipelineCreateInfo& key : *pipelines)
        {
            if (const auto pipeline = cached_graphics_pipelines.Find(key))
            {
                SDL_ReleaseGPUGraphicsPipeline(render_device->GetRawDevice(), *pipeline);
                cached_graphics_pipelines.Remove(key);
            }
        }
        graphics_program_to_pipeline_map.Remove(shader_program);
    }

    // Compute 파이프라인 무효화
    if (const auto pipelines = compute_program_to_pipeline_map.Find(shader_program))
    {
        for (const ComputePipelineCreateInfo& key : *pipelines)
        {
            if (const auto pipeline = cached_compute_pipelines.Find(key))
            {
                SDL_ReleaseGPUComputePipeline(render_device->GetRawDevice(), *pipeline);
                cached_compute_pipelines.Remove(key);
            }
        }
        compute_program_to_pipeline_map.Remove(shader_program);
    }

    // 셰이더 라이브러리에서 제거
    shader_library.Invalidate(shader_program);
}

void PSOManager::ClearAll()
{
    for (SDL_GPUGraphicsPipeline* pipeline : cached_graphics_pipelines | std::views::values)
    {
        SDL_ReleaseGPUGraphicsPipeline(render_device->GetRawDevice(), pipeline);
    }
    cached_graphics_pipelines.Clear();
    graphics_program_to_pipeline_map.Clear();

    for (SDL_GPUComputePipeline* pipeline : cached_compute_pipelines | std::views::values)
    {
        SDL_ReleaseGPUComputePipeline(render_device->GetRawDevice(), pipeline);
    }
    cached_compute_pipelines.Clear();
    compute_program_to_pipeline_map.Clear();

    shader_library.ClearAll();
}
} // namespace se
