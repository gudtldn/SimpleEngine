#include "Graphics/Compiler/Compiler.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#if SE_HAS_HLSL_COMPILER
#include <ranges>
#endif


namespace se::editor
{
ShaderCompileResult<Array<uint8>> CompileHLSLToSPIRV(
    const Path& hlsl_path,
    StringView entrypoint,
    SDL_ShaderCross_ShaderStage stage,
    Optional<const Path&> include_dir_opt,
    Optional<ArrayView<const HLSL_Define>> defines_opt
)
{
#if SE_HAS_HLSL_COMPILER
    // read shader file
    Array<uint8> source;
    if (auto result = FileSystem::ReadBytes(hlsl_path))
    {
        source = std::move(result).Value();
        source.Emplace('\0'); // null-terminated
    }
    else
    {
        return Unexpected<ShaderCompileError>{
            ShaderCompileError::ReadFailed,
            String::Format("Failed to read shader file: {}, Err: {}", hlsl_path, result.Error().What()),
            hlsl_path
        };
    }

    // build defines array
    Array<SDL_ShaderCross_HLSL_Define> hlsl_defines;
    if (defines_opt)
    {
        const ArrayView<const HLSL_Define> defines = *defines_opt;
        hlsl_defines.ResizeUninitialized(defines.Len());

        for (auto [n, hlsl_define] : hlsl_defines | std::views::enumerate)
        {
            hlsl_define.name = const_cast<char*>(defines[n].name);
            hlsl_define.value = const_cast<char*>(defines[n].value);
        }
    }

    // entrypoint -> null-terminated string
    const String entrypoint_str = entrypoint;

    const SDL_ShaderCross_HLSL_Info hlsl_info = {
        .source = reinterpret_cast<const char*>(source.Data()),
        .entrypoint = entrypoint_str.CStr(),
        .include_dir = include_dir_opt ? include_dir_opt->CStr() : nullptr,
        .defines = defines_opt ? hlsl_defines.Data() : nullptr,
        .shader_stage = stage,
    };

    // HLSL -> SPIR-V
    usize spirv_size = 0;
    void* spirv_bytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &spirv_size);
    if (!spirv_bytecode)
    {
        return Unexpected<ShaderCompileError>{
            ShaderCompileError::CompileFailed,
            String::Format("Failed to compile HLSL to SPIR-V: {} (entry: {}), Err: {}",hlsl_path, entrypoint, SDL_GetError()),
            hlsl_path
        };
    }

    // SPIR-V 바이트를 Array<uint8>로 복사
    Array<uint8> result;
    result.ResizeUninitialized(spirv_size);
    std::memcpy(result.Data(), spirv_bytecode, spirv_size);
    SDL_free(spirv_bytecode);

    return result;

#else
    return Unexpected<ShaderCompileError>{
        ShaderCompileError::NotSupported,
        String::Format("HLSL compilation is not available on this platform: {}", hlsl_path),
        hlsl_path
    };
#endif
}
} // namespace se::editor
