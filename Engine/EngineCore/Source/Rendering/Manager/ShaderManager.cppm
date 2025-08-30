export module SimpleEngine.Rendering:Manager.ShaderManager;
import :ShaderProvider.IShaderProvider;

import SimpleEngine.Types;
import std;

import "SDL3/SDL_gpu.h";


namespace se::rendering::manager
{
using namespace shader_provider;

/**
 * Rendering에 사용될 셰이더를 관리하는 매니저
 */
export class ShaderManager
{
public:
    explicit ShaderManager(SDL_GPUDevice* in_device);
    ~ShaderManager();

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    /** Shader를 컴파일하는데 사용되는 Provider를 변경합니다. */
    template <typename T>
        requires std::derived_from<T, IShaderProvider>
    void SetProvider();

    /** SDL_GPUShader* 를 가져옵니다. */
    [[nodiscard]] SDL_GPUShader* GetShader(const ShaderRequest& request);

private:
    SDL_GPUDevice* device;
    std::unique_ptr<IShaderProvider> provider;

    std::unordered_map<ShaderRequest, SDL_GPUShader*> shader_cache;
};

template <typename T>
    requires std::derived_from<T, IShaderProvider>
void ShaderManager::SetProvider()
{
    provider = std::make_unique<T>();
}
}
