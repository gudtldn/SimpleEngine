#pragma once
#include <filesystem>

#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/Containers/Optional.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor::rendering
{
struct HLSL_Define
{
    const char* name;  // The define name.
    const char* value; // An optional value for the define. Can be NULL.
};

/** HLSL을 컴파일하여 SDL_GPUShader로 변환합니다. */
[[nodiscard]] SDL_GPUShader* CompileFromHLSL(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path,
    Optional<const std::filesystem::path&> include_dir_opt = std::nullopt,
    Optional<const vector<HLSL_Define>&> defines_opt = std::nullopt
);

// TODO: CreateComputeShader 구현하기
}
