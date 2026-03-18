#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/Manager/ShaderCache.h"
#include "SimpleEngine/Graphics/Traits/CreateInfoHash.h"

#include "SDL3/SDL_gpu.h"

#include <concepts>


namespace se::graphics
{
/**
 * Graphics API에 사용되는 PSO를 관리하는 매니저
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

    /** Shader를 컴파일하는데 사용되는 Provider를 변경합니다. */
    template <typename T, typename... Args>
        requires std::derived_from<T, IShaderProvider>
    void SetShaderCacheProvider(Args&&... args);

    /** Shader 캐시 정리 및 파이프라인 정리를 수행합니다. */
    void EndFrame();

private:
    RenderDevice* render_device;
    ShaderCache shader_cache;

    HashMap<GraphicsPipelineCreateInfo, SDL_GPUGraphicsPipeline*> cached_graphics_pipelines;
    HashMap<ComputePipelineCreateInfo, SDL_GPUComputePipeline*> cached_compute_pipelines;
};

template <typename T, typename... Args>
    requires std::derived_from<T, IShaderProvider>
void PSOManager::SetShaderCacheProvider(Args&&... args)
{
    shader_cache.SetProvider<T>(std::forward<Args>(args)...);
}
}  // namespace se::graphics
