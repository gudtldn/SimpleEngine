module;
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>
export module SE.Utility:ShaderUtils;
import :FileUtils;

import SE.Types;
import std;


export namespace se::utility::shader_utils
{
/** 파일명으로 ShaderStage를 자동으로 탐지합니다. */
[[nodiscard]] Optional<SDL_ShaderCross_ShaderStage> DetermineShaderStage(const std::filesystem::path& shader_path);

/** SPIRV를 컴파일하여 SDL_GPUShader로 변환합니다. */
[[nodiscard]] SDL_GPUShader* CompileFromSPIRV(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path
);
}
