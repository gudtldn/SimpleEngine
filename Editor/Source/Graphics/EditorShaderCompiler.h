#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "Graphics/ShaderCompileError.h"

#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::editor
{
/**
 * HLSL 소스에서 파싱된 셰이더 진입점 정보
 * #pragma se_shader <stage> <entry> 형태로 선언됩니다.
 */
struct ShaderEntryPoint
{
    SDL_ShaderCross_ShaderStage stage;
    String entrypoint;
};

/**
 * 셰이더 컴파일 결과 (진입점 하나당 하나)
 */
struct ShaderCompileOutput
{
    String output_stem;  // 출력 파일명의 stem (예: "Default.vert")
    Array<uint8> spirv_bytecode;
};

/**
 * 에디터 전용 셰이더 컴파일 관리자
 *
 * #pragma se_shader <stage> <entry> 형태의 pragma를 파싱하여
 * 하나의 HLSL 파일에서 여러 셰이더를 컴파일할 수 있습니다.
 * pragma가 없으면 파일명(.vert/.frag/.comp)에서 스테이지를 추론하고 entry="main"을 사용합니다.
 */
struct EditorShaderCompiler
{
    EditorShaderCompiler() = delete;

    /**
     * 셰이더 소스 디렉토리의 모든 .hlsl 파일을 .spv로 컴파일하여 디스크에 저장합니다.
     * @param hlsl_dir HLSL 소스 디렉토리
     * @param output_dir SPIR-V 출력 디렉토리
     */
    static void CompileAll(const Path& hlsl_dir, const Path& output_dir);

    /**
     * 단일 HLSL 파일을 컴파일합니다.
     * 여러 엔트리포인트를 각각 컴파일하여 여러개의 결과를 반환합니다.
     */
    [[nodiscard]] static ShaderCompileResult<Array<ShaderCompileOutput>> CompileShader(const Path& hlsl_path);
};
} // namespace se::editor
