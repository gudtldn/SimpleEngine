module;
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>
module SimpleEngine.Editor.Utility;
import :ShaderUtils;

import SimpleEngine.Core;
import SimpleEngine.Types;
import SimpleEngine.Utility;


namespace
{
/** 파일명으로 ShaderStage를 자동으로 탐지합니다. */
[[nodiscard]] Optional<SDL_ShaderCross_ShaderStage> DetermineShaderStage(const std::filesystem::path& shader_path)
{
    const std::u8string path_str = shader_path.generic_u8string();

    if (
        path_str.find(u8".vert") != std::string::npos
        || path_str.find(u8".vertex") != std::string::npos
        || path_str.find(u8".vs") != std::string::npos
    )
    {
        return SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    }

    if (
        path_str.find(u8".frag") != std::string::npos
        || path_str.find(u8".fragment") != std::string::npos
        || path_str.find(u8".fs") != std::string::npos
        || path_str.find(u8".pixel") != std::string::npos
        || path_str.find(u8".ps") != std::string::npos
    )
    {
        return SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    }

    if (
        path_str.find(u8".comp") != std::string::npos
        || path_str.find(u8".compute") != std::string::npos
        || path_str.find(u8".cs") != std::string::npos
    )
    {
        return SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
    }

    return std::nullopt;
}
}

namespace se::editor::utility::shader_utils
{
using namespace se::utility;

SDL_GPUShader* CompileSPIRV(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path,
    uint32 sampler_count,
    uint32 uniform_buffer_count,
    uint32 storage_buffer_count,
    uint32 storage_texture_count
)
{
    // read shader file
    std::vector<uint8> source;
    if (auto result = file_utils::ReadToByteArray(shader_path))
    {
        source = std::move(result).value();
        source.emplace_back(0); // null-terminated
    }
    else
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to read shader file: {}, Err: {}", shader_path.generic_u8string(), result.error().message);
        return nullptr;
    }

    // define default info
    const char* entrypoint = "main";
    const Optional<SDL_ShaderCross_ShaderStage> stage_opt = DetermineShaderStage(shader_path);

    if (!stage_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to determine shader stage: {}", shader_path.generic_u8string());
        return nullptr;
    }

    SDL_GPUShaderStage stage;
    switch (stage_opt.Value())
    {
    case SDL_SHADERCROSS_SHADERSTAGE_VERTEX:
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
        break;
    case SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT:
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        break;
    default:
        ConsoleLog(ELogLevel::Error, u8"Unknown shader stage: {}", shader_path.generic_u8string()); // Compute Shader는 다른 함수로
        return nullptr;
    }

    // compile shader
    const SDL_ShaderCross_SPIRV_Info spirv_info = {
        .bytecode = source.data(),
        .bytecode_size = source.size(),
        .entrypoint = entrypoint,
        .shader_stage = *stage_opt,
        .enable_debug = true,
    };

    // create gpu shader
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        size_t bytecode_size;
        void* bytecode = SDL_ShaderCross_CompileDXILFromSPIRV(&spirv_info, &bytecode_size);

        const SDL_GPUShaderCreateInfo create_info = {
            .code_size = bytecode_size,
            .code = static_cast<const Uint8*>(bytecode),
            .entrypoint = entrypoint,
            .format = backend_formats,
            .stage = stage,
            .num_samplers = sampler_count,
            .num_storage_textures = storage_texture_count,
            .num_storage_buffers = storage_buffer_count,
            .num_uniform_buffers = uniform_buffer_count,
        };
        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &create_info);
        SDL_free(bytecode);
        return shader;
    }

    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        const SDL_ShaderCross_GraphicsShaderMetadata metadata = {
            .num_samplers = sampler_count,
            .num_storage_textures = storage_texture_count,
            .num_storage_buffers = storage_buffer_count,
            .num_uniform_buffers = uniform_buffer_count,
        };
        return SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &spirv_info, &metadata, 0);
    }

    ConsoleLog(ELogLevel::Error, u8"Unknown shader backend format: {}", shader_path.generic_u8string());
    return nullptr;
}

SDL_GPUShader* CompileHLSL(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path,
    Optional<const std::filesystem::path&> include_dir_opt,
    Optional<const std::vector<HLSL_Define>&> defines_opt,
    uint32 sampler_count,
    uint32 uniform_buffer_count,
    uint32 storage_buffer_count,
    uint32 storage_texture_count
)
{
    // read shader file
    std::vector<uint8> source;
    if (auto result = file_utils::ReadToByteArray(shader_path))
    {
        source = std::move(result).value();
        source.emplace_back(0); // null-terminated
    }
    else
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to read shader file: {}, Err: {}", shader_path.generic_u8string(), result.error().message);
        return nullptr;
    }

    // define default info
    const char* entrypoint = "main";
    const Optional<SDL_ShaderCross_ShaderStage> stage_opt = DetermineShaderStage(shader_path);

    if (!stage_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to determine shader stage: {}", shader_path.generic_u8string());
        return nullptr;
    }

    SDL_GPUShaderStage stage;
    switch (stage_opt.Value())
    {
    case SDL_SHADERCROSS_SHADERSTAGE_VERTEX:
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
        break;
    case SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT:
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        break;
    default:
        ConsoleLog(ELogLevel::Error, u8"Unknown shader stage: {}", shader_path.generic_u8string()); // Compute Shader는 다른 함수로
        return nullptr;
    }

    // compile shader
    std::string include_dir_str;
    if (include_dir_opt)
    {
        include_dir_str = include_dir_opt->generic_string();
    }

    std::vector<SDL_ShaderCross_HLSL_Define> hlsl_defines;
    if (defines_opt)
    {
        const std::vector<HLSL_Define>& defines = *defines_opt;
        hlsl_defines.resize(defines.size());

        for (auto [n, hlsl_define] : hlsl_defines | std::ranges::views::enumerate)
        {
            hlsl_define.name = const_cast<char*>(defines[n].name);
            hlsl_define.value = const_cast<char*>(defines[n].value);
        }
    }

    const SDL_ShaderCross_HLSL_Info hlsl_info = {
        .source = reinterpret_cast<const char*>(source.data()),
        .entrypoint = entrypoint,
        .include_dir = include_dir_opt ? include_dir_str.c_str() : nullptr,
        .defines = defines_opt ? hlsl_defines.data() : nullptr,
        .shader_stage = *stage_opt,
        .enable_debug = IS_DEBUG_BUILD,
    };

    void* bytecode = nullptr;
    size_t bytecode_size = 0;

    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        bytecode = SDL_ShaderCross_CompileDXILFromHLSL(&hlsl_info, &bytecode_size);
    }
    else if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        bytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &bytecode_size);
    }

    // create gpu shader
    if (bytecode)
    {
        const SDL_GPUShaderCreateInfo create_info = {
            .code_size = bytecode_size,
            .code = static_cast<const Uint8*>(bytecode),
            .entrypoint = entrypoint,
            .format = backend_formats,
            .stage = stage,
            .num_samplers = sampler_count,
            .num_storage_textures = storage_texture_count,
            .num_storage_buffers = storage_buffer_count,
            .num_uniform_buffers = uniform_buffer_count,
        };
        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &create_info);
        SDL_free(bytecode);
        return shader;
    }

    ConsoleLog(ELogLevel::Error, u8"Unknown shader backend format: {}, Err: {}", shader_path.generic_u8string(), SDL_GetError());
    return nullptr;
}
}
