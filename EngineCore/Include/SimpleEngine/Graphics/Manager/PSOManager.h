#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/Manager/ShaderCache.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoHash.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
/**
 * Graphics API에 사용되는 PSO를 관리하는 매니저
 * 셰이더 캐시를 직접 소유하며, VPath 기반 논리 키로 동작합니다.
 */
class SE_CORE_API PSOManager
{
public:
    explicit PSOManager(RenderDevice& in_render_device);
    ~PSOManager();

    /** 캐싱된 SDL_GPUGraphicsPipeline* 를 가져오거나, 새로 생성합니다. */
    [[nodiscard]] SDL_GPUGraphicsPipeline* GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

    /** 캐싱된 SDL_GPUComputePipeline* 를 가져오거나, 새로 생성합니다. */
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info);

    /** 특정 셰이더를 무효화하고, 해당 셰이더를 사용하는 파이프라인도 모두 제거합니다. (핫 리로드용) */
    void InvalidateShader(const VPath& shader_key);

private:
    RenderDevice* render_device;
    ShaderCache shader_cache;

    HashMap<GraphicsPipelineCreateInfo, SDL_GPUGraphicsPipeline*> cached_graphics_pipelines;
    HashMap<ComputePipelineCreateInfo, SDL_GPUComputePipeline*> cached_compute_pipelines;

    // 셰이더 -> 파이프라인 역추적 인덱스 (핫 리로드 시 관련 파이프라인 자동 삭제)
    HashMap<VPath, Array<GraphicsPipelineCreateInfo>> graphics_shader_to_pipeline_map;
    HashMap<VPath, Array<ComputePipelineCreateInfo>> compute_shader_to_pipeline_map;
};
}  // namespace se::graphics
