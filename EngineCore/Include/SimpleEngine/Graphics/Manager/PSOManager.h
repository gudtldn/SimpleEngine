#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoHash.h"
#include "SimpleEngine/Shader/ShaderLibrary.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
// forward declaration
class IShaderBundleSource;
class RenderDevice;

/**
 * Graphics API에 사용되는 PSO를 관리하는 매니저
 * 셰이더 라이브러리를 직접 소유하며, 셰이더 프로그램 VPath를 키로 동작합니다.
 */
class SE_CORE_API PSOManager
{
public:
    PSOManager(RenderDevice& in_render_device, const IShaderBundleSource& shader_bundle_source);
    ~PSOManager();

    /** 캐싱된 SDL_GPUGraphicsPipeline* 를 가져오거나, 새로 생성합니다. */
    [[nodiscard]] SDL_GPUGraphicsPipeline* GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

    /** 캐싱된 SDL_GPUComputePipeline* 를 가져오거나, 새로 생성합니다. */
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info);

    /** 셰이더 프로그램을 무효화하고, 그 프로그램을 사용하는 파이프라인도 모두 제거합니다. (핫 리로드용) */
    void InvalidateShader(const VPath& shader_program);

    /** 모든 셰이더와 파이프라인 캐시를 비웁니다. (핫 리로드용 전체 무효화) */
    void ClearAll();

private:
    RenderDevice* render_device;
    ShaderLibrary shader_library;

    HashMap<GraphicsPipelineCreateInfo, SDL_GPUGraphicsPipeline*> cached_graphics_pipelines;
    HashMap<ComputePipelineCreateInfo, SDL_GPUComputePipeline*> cached_compute_pipelines;

    // 셰이더 프로그램 -> 파이프라인 역추적 인덱스 (핫 리로드 시 관련 파이프라인 자동 삭제)
    HashMap<VPath, Array<GraphicsPipelineCreateInfo>> graphics_program_to_pipeline_map;
    HashMap<VPath, Array<ComputePipelineCreateInfo>> compute_program_to_pipeline_map;
};
} // namespace se
