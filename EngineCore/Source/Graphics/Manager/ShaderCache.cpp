#include "SimpleEngine/Graphics/Manager/ShaderCache.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/ShaderUtils.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include <ranges>


namespace se::graphics
{
ShaderCache::ShaderCache(RenderDevice& in_render_device)
    : render_device(&in_render_device)
{
}

ShaderCache::~ShaderCache()
{
    ClearAll();
}

SDL_GPUShader* ShaderCache::GetOrCreateShader(const VPath& shader_key, SDL_ShaderCross_ShaderStage stage)
{
    if (const auto cache = graphics_cache.Find(shader_key))
    {
        return *cache;
    }

    auto spirv_opt = ReadSpvFile(shader_key);
    if (!spirv_opt.HasValue())
    {
        return nullptr;
    }

    SDL_GPUShader* shader = CreateGraphicsShader(*render_device, stage, *spirv_opt);
    if (!shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create graphics shader: {}", shader_key);
        return nullptr;
    }

    graphics_cache[shader_key] = shader;
    return shader;
}

void ShaderCache::LoadShaderFromMemory(const VPath& shader_key, SDL_ShaderCross_ShaderStage stage, ArrayView<const uint8> spirv_bytecode)
{
    if (const auto cache = graphics_cache.Find(shader_key))
    {
        SDL_ReleaseGPUShader(render_device->GetRawDevice(), *cache);
        graphics_cache.Remove(shader_key);
    }

    SDL_GPUShader* shader = CreateGraphicsShader(*render_device, stage, spirv_bytecode);
    if (!shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to load graphics shader from memory: {}", shader_key);
        return;
    }

    graphics_cache[shader_key] = shader;
}

void ShaderCache::Invalidate(const VPath& shader_key)
{
    if (const auto cache = graphics_cache.Find(shader_key))
    {
        SDL_ReleaseGPUShader(render_device->GetRawDevice(), *cache);
        graphics_cache.Remove(shader_key);
    }
}

void ShaderCache::ClearAll()
{
    for (SDL_GPUShader* shader : graphics_cache | std::views::values)
    {
        SDL_ReleaseGPUShader(render_device->GetRawDevice(), shader);
    }
    graphics_cache.Clear();
}

Path ShaderCache::ResolveSpvPath(const VPath& shader_key)
{
    // "CoreShader://DebugLine.vert" -> "CoreShader://Compiled/DebugLine.vert.spv"
    const VPath parent = shader_key.GetParentPath();
    const String filename = shader_key.GetFilename();

    const VPath spv_vpath = parent / "Compiled" / (filename + ".spv");
    return VFS::ToPath(spv_vpath);
}

Optional<Array<uint8>> ShaderCache::ReadSpvFile(const VPath& shader_key)
{
    const Path spv_path = ResolveSpvPath(shader_key);
    auto result = FileSystem::ReadBytes(spv_path);
    if (!result.HasValue())
    {
        ConsoleLog(
            ELogLevel::Error,
            "Failed to read .spv file: {} (resolved: {}), Err: {}", shader_key, spv_path, result.Error().What()
        );
        return NullOpt;
    }
    return std::move(result).Value();
}
} // namespace se::graphics