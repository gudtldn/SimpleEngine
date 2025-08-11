module;
#include <SDL3/SDL_gpu.h>
module SimpleEngine.Utility;
import :ShaderUtils;

import SimpleEngine.Core;


namespace se::utility::shader_utils
{
SDL_GPUShader* LoadCompiledShader(
    SDL_GPUDevice* device, const std::filesystem::path& shader_path, uint32 sampler_count, uint32 uniform_buffer_count, uint32 storage_buffer_count,
    uint32 storage_texture_count
)
{
    auto data = file_utils::ReadToByteArray(shader_path);
    if (!data.has_value())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to read shader file: {}, Err: {}", shader_path.generic_u8string(), data.error().message);
        return nullptr;
    }

    const std::u8string path = shader_path.generic_u8string();

    // 파일 확장자로 stage 구분
    SDL_GPUShaderStage stage;
    if (path.find(u8".vert") != std::string::npos)
    {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (path.find(u8".frag") != std::string::npos)
    {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        ConsoleLog(ELogLevel::Error, u8"Unknown shader file extension: {}", path);
        return nullptr;
    }

    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    const char* entrypoint;

    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        entrypoint = "main";
    }
    else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL)
    {
        entrypoint = "main0";
    }
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        entrypoint = "main";
    }
    else
    {
        ConsoleLog(ELogLevel::Error, u8"Unknown shader backend format: {}", path);
        return nullptr;
    }

    const SDL_GPUShaderCreateInfo info = {
        .code_size = data->size(),
        .code = data->data(),
        .entrypoint = entrypoint,
        .format = backend_formats,
        .stage = stage,
        .num_samplers = sampler_count,
        .num_storage_textures = storage_texture_count,
        .num_storage_buffers = storage_buffer_count,
        .num_uniform_buffers = uniform_buffer_count,
    };

    if (SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info))
    {
        return shader;
    }

    ConsoleLog(ELogLevel::Error, u8"Failed to create shader: {}", SDL_GetError());
    return nullptr;
}
}
