module;
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>
module SimpleEngine.Utility;
import :ShaderUtils;

import SimpleEngine.Core;


namespace se::utility::shader_utils
{
Optional<SDL_ShaderCross_ShaderStage> DetermineShaderStage(const std::filesystem::path& shader_path)
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

SDL_GPUShader* CompileFromSPIRV(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path
)
{
    // read shader file
    std::vector<uint8> source;
    if (auto result = file_utils::ReadToByteArray(shader_path))
    {
        source = std::move(result).value();
        source.emplace_back('\0'); // null-terminated
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

    // get reflection metadata
    const SDL_ShaderCross_GraphicsShaderMetadata* refl_metadata =
        SDL_ShaderCross_ReflectGraphicsSPIRV(source.data(), source.size(), 0);

    if (!refl_metadata)
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to reflect shader: {}", shader_path.generic_u8string());
        return nullptr;
    }

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
            .num_samplers = refl_metadata->num_samplers,
            .num_storage_textures = refl_metadata->num_storage_textures,
            .num_storage_buffers = refl_metadata->num_storage_buffers,
            .num_uniform_buffers = refl_metadata->num_uniform_buffers,
        };
        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &create_info);
        SDL_free(bytecode);
        return shader;
    }

    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        const SDL_ShaderCross_GraphicsShaderMetadata metadata = {
            .num_samplers = refl_metadata->num_samplers,
            .num_storage_textures = refl_metadata->num_storage_textures,
            .num_storage_buffers = refl_metadata->num_storage_buffers,
            .num_uniform_buffers = refl_metadata->num_uniform_buffers,
            .num_inputs = refl_metadata->num_inputs,
            .inputs = refl_metadata->inputs,
            .num_outputs = refl_metadata->num_outputs,
            .outputs = refl_metadata->outputs,
        };
        return SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &spirv_info, &metadata, 0);
    }

    ConsoleLog(ELogLevel::Error, u8"Unknown shader backend format: {}", shader_path.generic_u8string());
    return nullptr;
}
}
