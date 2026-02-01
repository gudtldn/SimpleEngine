#pragma once
#include <concepts>
#include <memory>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Graphics/ShaderProvider/PrecompiledShaderProvider.h"


namespace se::graphics
{
/**
 * Rendering에 사용될 셰이더를 관리하는 매니저
 */
class SE_CORE_API ShaderCache
{
public:
    explicit ShaderCache(
        SDL_GPUDevice* in_device,
        std::unique_ptr<IShaderProvider> init_provider = std::make_unique<PrecompiledShaderProvider>()
    );
    ~ShaderCache();

    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = delete;
    ShaderCache& operator=(ShaderCache&&) = delete;

    /** Shader를 컴파일하는데 사용되는 Provider를 변경합니다. */
    template <typename T, typename... Args>
        requires std::derived_from<T, IShaderProvider>
    void SetProvider(Args&&... args);

    /** SDL_GPUShader* 를 가져옵니다. */
    [[nodiscard]] SDL_GPUShader* GetOrCreate(const ShaderRequest& request);

    /** 캐시를 모두 해제 후 삭제합니다. */
    void ClearCache();

private:
    SDL_GPUDevice* device;
    std::unique_ptr<IShaderProvider> provider;

    HashMap<ShaderRequest, SDL_GPUShader*> shader_cache;
};

template <typename T, typename... Args>
    requires std::derived_from<T, IShaderProvider>
void ShaderCache::SetProvider(Args&&... args)
{
    provider = std::make_unique<T>(std::forward<Args>(args)...);
}
}  // namespace se::graphics
