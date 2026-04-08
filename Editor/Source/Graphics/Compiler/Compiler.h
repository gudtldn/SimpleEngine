#pragma once

#include "SimpleEditor/Graphics/ShaderCompileError.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::editor
{
/**
 * HLSL 전처리기 매크로 정의
 * SDL_ShaderCross_HLSL_Define와 1:1 대응합니다.
 */
struct HLSL_Define
{
    const char* name;  // 매크로 이름
    const char* value; // 매크로 값 (없으면 nullptr)
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
