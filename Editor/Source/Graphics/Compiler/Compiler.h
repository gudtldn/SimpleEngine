#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "SDL3/SDL_gpu.h"


// forward declaration
namespace se::graphics{ class RenderDevice; }

namespace se::editor
{
struct HLSL_Define
{
    const char* name;  // The define name.
    const char* value; // An optional value for the define. Can be NULL.
};

/** HLSL을 컴파일하여 SDL_GPUShader로 변환합니다. */
[[nodiscard]] SDL_GPUShader* CompileFromHLSL(
    const se::graphics::RenderDevice& render_device,
    const Path& shader_path,
    Optional<const Path&> include_dir_opt = NullOpt,
    Optional<ArrayView<const HLSL_Define>> defines_opt = NullOpt
);

// TODO: CreateComputeShader 구현하기
}  // namespace se::editor
