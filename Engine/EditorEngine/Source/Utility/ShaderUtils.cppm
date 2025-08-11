module;
#include <SDL3/SDL_gpu.h>
export module SimpleEngine.Editor.Utility:ShaderUtils;

import SimpleEngine.Types;
import std;


export namespace se::editor::utility::shader_utils
{
struct HLSL_Define
{
    const char* name;  // The define name.
    const char* value; // An optional value for the define. Can be NULL.
};

/** SPIRV를 컴파일하여 SDL_GPUShader로 변환합니다. */
[[nodiscard]] SDL_GPUShader* CompileSPIRV(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path,
    uint32 sampler_count,
    uint32 uniform_buffer_count,
    uint32 storage_buffer_count,
    uint32 storage_texture_count
);

/** HLSL을 컴파일하여 SDL_GPUShader로 변환합니다. */
[[nodiscard]] SDL_GPUShader* CompileHLSL(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path,
    Optional<const std::filesystem::path&> include_dir_opt,
    Optional<const std::vector<HLSL_Define>&> defines_opt,
    uint32 sampler_count,
    uint32 uniform_buffer_count,
    uint32 storage_buffer_count,
    uint32 storage_texture_count
);

// TODO: CreateComputeShader 구현하기
}
