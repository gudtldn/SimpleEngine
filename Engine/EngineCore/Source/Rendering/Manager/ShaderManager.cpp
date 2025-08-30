module SimpleEngine.Rendering;
import :Manager.ShaderManager;

import SimpleEngine.Core;



namespace se::rendering::manager
{
ShaderManager::ShaderManager(SDL_GPUDevice* in_device, std::unique_ptr<IShaderProvider> init_provider)
    : device(in_device)
    , provider(std::move(init_provider))
{
}

ShaderManager::~ShaderManager()
{
    for (SDL_GPUShader* cache : shader_cache | std::views::values)
    {
        SDL_ReleaseGPUShader(device, cache);
        cache = nullptr;
    }
    shader_cache.clear();
}

SDL_GPUShader* ShaderManager::GetShader(const ShaderRequest& request)
{
    if (shader_cache.contains(request))
    {
        return shader_cache[request];
    }

    if (SDL_GPUShader* shader = provider->Provide(device, request))
    {
        shader_cache[request] = shader;
        return shader;
    }

    ConsoleLog(ELogLevel::Error, u8"Failed to get shader from provider!");
    return nullptr;
}
}
