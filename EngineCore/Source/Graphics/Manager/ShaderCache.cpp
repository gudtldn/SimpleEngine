#include "SimpleEngine/Graphics/Manager/ShaderCache.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/ShaderUtils.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include <cstdlib>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>


namespace se
{
namespace
{
// [Slang 실험 전용] JSON 텍스트에서 `"key": <정수>`를 찾아 읽습니다.
u32 ReadSpikeJsonU32(const std::string& json, const char* key)
{
    const size_t pos = json.find(std::string("\"") + key + "\":");
    return pos == std::string::npos ? 0 : static_cast<u32>(std::strtoul(json.c_str() + json.find(':', pos) + 1, nullptr, 10));
}

// [Slang 실험 전용] 환경 변수 SE_SLANG_SPIKE_DIR이 있으면 SlangSpike가 만든 바이트코드와 JSON으로 셰이더를 만듭니다.
// shadercross를 거치지 않고 SDL_CreateGPUShader를 직접 부르며, 리소스 개수는 Slang 리플렉션 값을 씁니다.
Optional<GraphicsShaderCreateResult> TryCreateSlangSpikeShader(
    const RenderDevice& render_device,
    const VPath& shader_key,
    SDL_ShaderCross_ShaderStage stage
)
{
    const char* spike_dir = std::getenv("SE_SLANG_SPIKE_DIR");
    if (!spike_dir)
    {
        return NullOpt;
    }

    SDL_GPUDevice* device = render_device.GetRawDevice();
    const bool use_dxil = (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_DXIL) != 0;
    const std::string base = std::string(spike_dir) + "/" + shader_key.GetFilename().CStr();

    std::ifstream code_file(base + (use_dxil ? ".dxil" : ".spv"), std::ios::binary);
    std::ifstream json_file(base + ".json");
    if (!code_file || !json_file)
    {
        return NullOpt;
    }

    const std::string code((std::istreambuf_iterator<char>(code_file)), std::istreambuf_iterator<char>());
    std::stringstream json_stream;
    json_stream << json_file.rdbuf();
    const std::string json = json_stream.str();

    const size_t entry_key = json.find("\"entry\": \"");
    if (entry_key == std::string::npos)
    {
        return NullOpt;
    }
    const size_t entry_begin = entry_key + 10;
    const std::string entry = json.substr(entry_begin, json.find('"', entry_begin) - entry_begin);

    const SDL_GPUShaderCreateInfo create_info = {
        .code_size = code.size(),
        .code = reinterpret_cast<const Uint8*>(code.data()),
        .entrypoint = entry.c_str(),
        .format = use_dxil ? SDL_GPU_SHADERFORMAT_DXIL : SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = ReadSpikeJsonU32(json, "samplers"),
        .num_storage_textures = ReadSpikeJsonU32(json, "storage_textures"),
        .num_storage_buffers = ReadSpikeJsonU32(json, "storage_buffers"),
        .num_uniform_buffers = ReadSpikeJsonU32(json, "uniform_buffers"),
    };

    GraphicsShaderCreateResult result;
    result.shader = SDL_CreateGPUShader(device, &create_info);
    if (!result.shader)
    {
        ConsoleLog(ELogLevel::Error, "Slang spike: failed to create shader {}, Err: {}", shader_key, SDL_GetError());
        return NullOpt;
    }

    // "vertex_inputs": [0, 1, 2]
    const size_t inputs_key = json.find("\"vertex_inputs\": [");
    if (inputs_key != std::string::npos)
    {
        const char* cursor = json.c_str() + inputs_key + 18;
        while (*cursor != ']' && *cursor != '\0')
        {
            char* next = nullptr;
            const unsigned long location = std::strtoul(cursor, &next, 10);
            if (next == cursor)
            {
                ++cursor;
                continue;
            }
            result.reflection.vertex_inputs.Push({ .location = static_cast<u32>(location) });
            cursor = next;
        }
    }

    ConsoleLog(ELogLevel::Info, "Slang spike: {} from {} ({})", shader_key, base, use_dxil ? "DXIL" : "SPIR-V");
    return result;
}
} // namespace

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

    if (auto spike_result = TryCreateSlangSpikeShader(*render_device, shader_key, stage))
    {
        graphics_cache.Insert(shader_key, spike_result->shader);
        reflection_cache.Insert(shader_key, std::move(spike_result->reflection));
        return spike_result->shader;
    }

    auto spirv_opt = ReadSpvFile(shader_key);
    if (!spirv_opt.HasValue())
    {
        return nullptr;
    }

    GraphicsShaderCreateResult result = CreateGraphicsShader(*render_device, stage, *spirv_opt);
    if (!result.shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create graphics shader: {}", shader_key);
        return nullptr;
    }

    graphics_cache.Insert(shader_key, result.shader);
    reflection_cache.Insert(shader_key, std::move(result.reflection));
    return result.shader;
}

void ShaderCache::LoadShaderFromMemory(const VPath& shader_key, SDL_ShaderCross_ShaderStage stage, ArrayView<const u8> spirv_bytecode)
{
    if (const auto cache = graphics_cache.Find(shader_key))
    {
        SDL_ReleaseGPUShader(render_device->GetRawDevice(), *cache);
        graphics_cache.Remove(shader_key);
    }

    GraphicsShaderCreateResult result = CreateGraphicsShader(*render_device, stage, spirv_bytecode);
    if (!result.shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to load graphics shader from memory: {}", shader_key);
        return;
    }

    graphics_cache.Insert(shader_key, result.shader);
    reflection_cache.Insert(shader_key, std::move(result.reflection));
}

void ShaderCache::Invalidate(const VPath& shader_key)
{
    if (const auto cache = graphics_cache.Find(shader_key))
    {
        SDL_ReleaseGPUShader(render_device->GetRawDevice(), *cache);
        graphics_cache.Remove(shader_key);
    }

    reflection_cache.Remove(shader_key);
}

void ShaderCache::ClearAll()
{
    for (SDL_GPUShader* shader : graphics_cache | std::views::values)
    {
        SDL_ReleaseGPUShader(render_device->GetRawDevice(), shader);
    }
    graphics_cache.Clear();
    reflection_cache.Clear();
}

Optional<const ShaderReflectionData&> ShaderCache::GetReflection(const VPath& shader_key) const
{
    return reflection_cache.Find(shader_key);
}

Path ShaderCache::ResolveSpvPath(const VPath& shader_key)
{
    // "CoreShader://DebugLine.vert" -> "CoreShader://Compiled/DebugLine.vert.spv"
    const VPath parent = shader_key.GetParentPath();
    const String filename = shader_key.GetFilename();

    const VPath spv_vpath = parent / "Compiled" / (filename + ".spv");
    return VFS::ToPath(spv_vpath);
}

Optional<Array<u8>> ShaderCache::ReadSpvFile(const VPath& shader_key)
{
    const Path spv_path = ResolveSpvPath(shader_key);
    auto result = fs::ReadBytes(spv_path);
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
} // namespace se
