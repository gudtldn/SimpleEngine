export module SimpleEngine.Rendering:Manager.PSOManager.ShaderCache;
import :ShaderProvider.IShaderProvider;
import :ShaderProvider.PrecompiledShaderProvider;

import SimpleEngine.Types;
import std;

import "SDL3/SDL_gpu.h";

using namespace se::rendering::shader_provider;


/**
 * Rendering에 사용될 셰이더를 관리하는 매니저
 */
class ShaderCache
{
public:
    explicit ShaderCache(
        SDL_GPUDevice* in_device,
        std::unique_ptr<IShaderCacheProvider> init_provider = std::make_unique<PrecompiledShaderProvider>()
    );
    ~ShaderCache();

    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = delete;
    ShaderCache& operator=(ShaderCache&&) = delete;

    /** Shader를 컴파일하는데 사용되는 Provider를 변경합니다. */
    template <typename T, typename... Args>
        requires std::derived_from<T, IShaderCacheProvider>
    void SetProvider(Args&&... args);

    /** SDL_GPUShader* 를 가져옵니다. */
    [[nodiscard]] SDL_GPUShader* GetOrCreate(const ShaderRequest& request);

    /** 캐시를 모두 해제 후 삭제합니다. */
    void ClearCache();

private:
    SDL_GPUDevice* device;
    std::unique_ptr<IShaderCacheProvider> provider;

    std::unordered_map<ShaderRequest, SDL_GPUShader*> shader_cache;
};

template <typename T, typename... Args>
    requires std::derived_from<T, IShaderCacheProvider>
void ShaderCache::SetProvider(Args&&... args)
{
    provider = std::make_unique<T>(std::forward<Args>(args)...);
}
