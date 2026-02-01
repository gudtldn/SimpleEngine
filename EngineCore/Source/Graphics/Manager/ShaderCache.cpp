#include "Graphics/Manager/ShaderCache.h"

#include <ranges>
#include "Core/Logging/Logging.h"


namespace se::graphics
{
ShaderCache::ShaderCache(SDL_GPUDevice* in_device, std::unique_ptr<IShaderProvider> init_provider)
    : device(in_device)
    , provider(std::move(init_provider))
{
}

ShaderCache::~ShaderCache()
{
    ClearCache();
}

SDL_GPUShader* ShaderCache::GetOrCreate(const ShaderRequest& request)
{
    if (Optional cache_opt = shader_cache.Find(request))
    {
        return *cache_opt;
    }

    if (SDL_GPUShader* shader = provider->Provide(device, request))
    {
        shader_cache[request] = shader;
        return shader;
    }

    ConsoleLog(ELogLevel::Error, "Failed to get shader from provider!");
    return nullptr;
}

void ShaderCache::ClearCache()
{
    for (SDL_GPUShader* cache : shader_cache | std::views::values)
    {
        SDL_ReleaseGPUShader(device, cache);
    }
    shader_cache.Clear();
}
}  // namespace se::graphics
