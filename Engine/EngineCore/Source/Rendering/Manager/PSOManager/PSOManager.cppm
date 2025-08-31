export module SimpleEngine.Rendering:Manager.PSOManager;
export import :Manager.PSOManager.PipelineCreateInfo;
export import :Manager.PSOManager.PipelineCreateInfoHash;
import :Manager.PSOManager.ShaderCache;

import SimpleEngine.Types;
import SimpleEngine.Utility;
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

    /** TODO: Docs */
    [[nodiscard]] SDL_GPUGraphicsPipeline* GetOrCreateGraphicsPipeline(const SDL_GPUGraphicsPipelineCreateInfo& create_info);

    /** TODO: Docs */
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const SDL_GPUComputePipelineCreateInfo& create_info);

    /** Shader를 컴파일하는데 사용되는 Provider를 변경합니다. */
    template <typename T, typename... Args>
        requires std::derived_from<T, IShaderCacheProvider>
    void SetShaderCacheProvider(Args&&... args);

    /** Shader 캐시 정리 및 파이프라인 정리를 수행합니다. */
    void EndFrame();

private:
    SDL_GPUDevice* device;
    ShaderCache shader_cache;
};

template <typename T, typename... Args>
    requires std::derived_from<T, IShaderCacheProvider>
void PSOManager::SetShaderCacheProvider(Args&&... args)
{
    shader_cache.SetProvider<T>(std::forward<Args>(args)...);
}
}

