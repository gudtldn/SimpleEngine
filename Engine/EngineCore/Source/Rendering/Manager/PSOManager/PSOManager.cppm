export module SE.Rendering:Manager.PSOManager;
export import :Manager.PSOManager.PipelineCreateInfo;
export import :Manager.PSOManager.CreateInfoHash;
import :Manager.PSOManager.ShaderCache;

import SE.Types;
import SE.Utility;
import std;

import "SDL3/SDL_gpu.h";


namespace se::rendering::manager
{
/**
 * Graphics API에 사용되는 PSO를 관리하는 매니저
 */
export class PSOManager
{
public:
    explicit PSOManager(SDL_GPUDevice* in_device);
    ~PSOManager();

    /** 캐싱된 SDL_GPUGraphicsPipeline* 를 가져오거나, 새로 생성합니다. */
    [[nodiscard]] SDL_GPUGraphicsPipeline* GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

    /** 캐싱된 SDL_GPUComputePipeline* 를 가져오거나, 새로 생성합니다. */
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info);

    /** Shader를 컴파일하는데 사용되는 Provider를 변경합니다. */
    template <typename T, typename... Args>
        requires std::derived_from<T, IShaderCacheProvider>
    void SetShaderCacheProvider(Args&&... args);

    /** Shader 캐시 정리 및 파이프라인 정리를 수행합니다. */
    void EndFrame();

private:
    SDL_GPUDevice* device;
    ShaderCache shader_cache;

    unordered_map<GraphicsPipelineCreateInfo, SDL_GPUGraphicsPipeline*> cached_graphics_pipelines;
    unordered_map<ComputePipelineCreateInfo, SDL_GPUComputePipeline*> cached_compute_pipelines;
};

template <typename T, typename... Args>
    requires std::derived_from<T, IShaderCacheProvider>
void PSOManager::SetShaderCacheProvider(Args&&... args)
{
    shader_cache.SetProvider<T>(std::forward<Args>(args)...);
}
}

