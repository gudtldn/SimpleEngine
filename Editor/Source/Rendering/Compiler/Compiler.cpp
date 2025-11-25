#include "Rendering/Compiler/Compiler.h"

#include <ranges>
#include "range/v3/view/enumerate.hpp"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/FileUtils.h"

#include "SDL3_shadercross/SDL_shadercross.h"
#include "SimpleEngine/Gfx/ShaderUtils.h"


namespace se::editor::rendering
{
using namespace se::utility;

SDL_GPUShader* CompileFromHLSL(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path,
    Optional<const std::filesystem::path&> include_dir_opt,
    Optional<const Array<HLSL_Define>&> defines_opt
)
{
    // read shader file
    Array<uint8> source;
    if (auto result = ReadToByteArray(shader_path))
    {
        source = std::move(result).Value();
        source.Emplace('\0'); // null-terminated
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "Failed to read shader file: {}, Err: {}", shader_path.generic_string(), result.Error().message);
        return nullptr;
    }

    // define default info
    const char* entrypoint = "main";
    const Optional<SDL_ShaderCross_ShaderStage> stage_opt = gfx::DetermineShaderStage(shader_path);

    if (!stage_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Failed to determine shader stage: {}", shader_path.generic_string());
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
        ConsoleLog(ELogLevel::Error, "Unknown shader stage: {}", shader_path.generic_string()); // Compute Shader는 다른 함수로
        return nullptr;
    }

    // compile shader
    std::string include_dir_str;
    if (include_dir_opt)
    {
        include_dir_str = include_dir_opt->generic_string();
    }

    Array<SDL_ShaderCross_HLSL_Define> hlsl_defines;
    if (defines_opt)
    {
        const Array<HLSL_Define>& defines = *defines_opt;
        hlsl_defines.Resize(defines.Len());

        for (auto [n, hlsl_define] : hlsl_defines | ranges::views::enumerate)
        {
            hlsl_define.name = const_cast<char*>(defines[n].name);
            hlsl_define.value = const_cast<char*>(defines[n].value);
        }
    }

    const SDL_ShaderCross_HLSL_Info hlsl_info = {
        .source = reinterpret_cast<const char*>(source.Data()),
        .entrypoint = entrypoint,
        .include_dir = include_dir_opt ? include_dir_str.c_str() : nullptr,
        .defines = defines_opt ? hlsl_defines.Data() : nullptr,
        .shader_stage = *stage_opt,
    };

    void* bytecode = nullptr;
    usize bytecode_size = 0;

    SDL_ShaderCross_GraphicsShaderMetadata* refl_metadata = nullptr;

    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        bytecode = SDL_ShaderCross_CompileDXILFromHLSL(&hlsl_info, &bytecode_size);

        // get reflection metadata
        usize spirv_size;
        void* spirv_bytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &spirv_size);
        refl_metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(static_cast<const Uint8*>(spirv_bytecode), spirv_size, 0);
        SDL_free(spirv_bytecode);
    }
    else if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        bytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &bytecode_size);
        refl_metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(static_cast<const Uint8*>(bytecode), bytecode_size, 0);
    }

    // create gpu shader
    if (bytecode && refl_metadata)
    {
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
        SDL_free(refl_metadata);
        SDL_free(bytecode);
        return shader;
    }

    ConsoleLog(ELogLevel::Error, "Unknown shader backend format: {}, Err: {}", shader_path.generic_string(), SDL_GetError());
    return nullptr;
}
}
