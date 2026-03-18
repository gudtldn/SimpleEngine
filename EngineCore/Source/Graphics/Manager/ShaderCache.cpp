#include "SimpleEngine/Graphics/Manager/ShaderCache.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include <ranges>


namespace se::graphics
{
ShaderCache::ShaderCache(RenderDevice& in_render_device, std::unique_ptr<IShaderProvider> init_provider)
    : render_device(&in_render_device)
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

    if (SDL_GPUShader* shader = provider->Provide(*render_device, request))
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
        SDL_ReleaseGPUShader(render_device->GetRawDevice(), cache);
    }
    shader_cache.Clear();
}
}  // namespace se::graphics
