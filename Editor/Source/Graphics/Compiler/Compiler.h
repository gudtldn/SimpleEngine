#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "Graphics/ShaderCompileError.h"

#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::editor
{
/** @todo docs */
struct HLSL_Define
{
    const char* name;  // The define name.
    const char* value; // An optional value for the define. Can be NULL.
};

/**
 * HLSL 소스를 SPIR-V 바이너리로 컴파일합니다.
 * DXC가 없는 플랫폼(ARM 등)에서는 NotSupported 에러를 반환합니다.
 *
 * @param hlsl_path HLSL 소스 파일 경로
 * @param entrypoint 셰이더 진입점 함수명 (예: "VSMain", "PSMain", "CSMain")
 * @param stage 셰이더 스테이지 (VERTEX / FRAGMENT / COMPUTE)
 * @param include_dir_opt include 디렉토리 (옵션)
 * @param defines_opt 전처리기 매크로 (옵션)
 */
[[nodiscard]] ShaderCompileResult<Array<uint8>> CompileHLSLToSPIRV(
    const Path& hlsl_path,
    StringView entrypoint,
    SDL_ShaderCross_ShaderStage stage,
    Optional<const Path&> include_dir_opt = NullOpt,
    Optional<ArrayView<const HLSL_Define>> defines_opt = NullOpt
);
} // namespace se::editor
