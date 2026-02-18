#include "SimpleEngine/Graphics/ShaderUtils.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Logging/Logging.h"


namespace se::graphics
{
Optional<SDL_ShaderCross_ShaderStage> DetermineShaderStage(const Path& shader_path)
{
    const String path_str = shader_path.ToString();

    if (
        path_str.Contains(".vert")
        || path_str.Contains(".vertex")
        || path_str.Contains(".vs")
    )
    {
        return SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    }

    if (
        path_str.Contains(".frag")
        || path_str.Contains(".fragment")
        || path_str.Contains(".fs")
        || path_str.Contains(".pixel")
        || path_str.Contains(".ps")
    )
    {
        return SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    }

    if (
        path_str.Contains(".comp")
        || path_str.Contains(".compute")
        || path_str.Contains(".cs")
    )
    {
        return SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
    }

    return std::nullopt;
}

SDL_GPUShader* CompileFromSPIRV(SDL_GPUDevice* device, const Path& shader_path)
{
    // read shader file
    Array<uint8> source;
    if (auto result = FileSystem::ReadBytes(shader_path))
    {
        source = std::move(result).Value();
        source.Emplace('\0'); // null-terminated
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "Failed to read shader file: {}, Err: {}", shader_path, result.Error().What());
        return nullptr;
    }

    // define default info
    const char* entrypoint = "main";
    const Optional<SDL_ShaderCross_ShaderStage> stage_opt = DetermineShaderStage(shader_path);

    if (!stage_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Failed to determine shader stage: {}", shader_path);
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
        ConsoleLog(ELogLevel::Error, "Unknown shader stage: {}", shader_path); // Compute Shader는 다른 함수로
        return nullptr;
    }

    // compile shader
    const SDL_ShaderCross_SPIRV_Info spirv_info = {
        .bytecode = source.Data(),
        .bytecode_size = source.Len(),
        .entrypoint = entrypoint,
        .shader_stage = *stage_opt,
    };

    // get reflection metadata
    const SDL_ShaderCross_GraphicsShaderMetadata* refl_metadata =
        SDL_ShaderCross_ReflectGraphicsSPIRV(source.Data(), source.Len(), 0);

    if (!refl_metadata)
    {
        ConsoleLog(ELogLevel::Error, "Failed to reflect shader: {}", shader_path);
        return nullptr;
    }

    // create gpu shader
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        usize bytecode_size;
        void* bytecode = SDL_ShaderCross_CompileDXILFromSPIRV(&spirv_info, &bytecode_size);

        const SDL_GPUShaderCreateInfo create_info = {
            .code_size = bytecode_size,
            .code = static_cast<const Uint8*>(bytecode),
            .entrypoint = entrypoint,
            .format = backend_formats,
            .stage = stage,
            .num_samplers = refl_metadata->resource_info.num_samplers,
            .num_storage_textures = refl_metadata->resource_info.num_storage_textures,
            .num_storage_buffers = refl_metadata->resource_info.num_storage_buffers,
            .num_uniform_buffers = refl_metadata->resource_info.num_uniform_buffers,
        };
        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &create_info);
        SDL_free(bytecode);
        return shader;
    }

    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        const SDL_ShaderCross_GraphicsShaderResourceInfo resource_info = {
            .num_samplers = refl_metadata->resource_info.num_samplers,
            .num_storage_textures = refl_metadata->resource_info.num_storage_textures,
            .num_storage_buffers = refl_metadata->resource_info.num_storage_buffers,
            .num_uniform_buffers = refl_metadata->resource_info.num_uniform_buffers,
        };
        return SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &spirv_info, &resource_info, 0);
    }

    ConsoleLog(ELogLevel::Error, "Unknown shader backend format: {}", shader_path);
    return nullptr;
}
}  // namespace se::graphics
