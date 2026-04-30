#include "Graphics/EditorShaderCompiler.h"
#include "Graphics/Compiler/Compiler.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Common.h"
#include "SimpleEngine/Utility/Debug.h"

#include "SDL3_shadercross/SDL_shadercross.h"


namespace se::editor
{
using namespace se;

namespace
{
Optional<SDL_ShaderCross_ShaderStage> ParseStageString(StringView str)
{
    if (str == "vertex")   { return SDL_SHADERCROSS_SHADERSTAGE_VERTEX;   }
    if (str == "fragment") { return SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT; }
    if (str == "compute")  { return SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;  }
    return NullOpt;
}

StringView StageToExtension(SDL_ShaderCross_ShaderStage stage)
{
    switch (stage)
    {
    case SDL_SHADERCROSS_SHADERSTAGE_VERTEX:   return "vert";
    case SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT: return "frag";
    case SDL_SHADERCROSS_SHADERSTAGE_COMPUTE:  return "comp";
    default:                                   return "unknown";
    }
}

/**
 * HLSL 소스 텍스트에서 #pragma se_shader 라인을 파싱합니다.
 * @return 파싱된 진입점 목록. pragma가 없으면 빈 배열을 반환합니다.
 */
[[nodiscard]] Array<ShaderEntryPoint> ParseShaderPragmas(StringView source)
{
    Array<ShaderEntryPoint> entries;
    constexpr StringView PRAGMA_PREFIX = "#pragma se_shader";

    usize pos = 0;
    while (pos < source.ByteLen())
    {
        const usize line_end = source.FindFirstOf('\n', pos).ValueOr(source.ByteLen());
        const StringView line = source.Substr(pos, line_end - pos).Trim();

        if (line.StartsWith(PRAGMA_PREFIX))
        {
            const StringView args = line.Substr(PRAGMA_PREFIX.ByteLen()).TrimStart();

            // "#pragma se_shader vertex VSMain" -> stage="vertex", entry="VSMain"
            if (const auto space = args.FindFirstOf(' '))
            {
                const StringView stage_str = args.Substr(0, *space);
                const StringView entry_str = args.Substr(*space + 1).Trim();

                const auto stage_opt = ParseStageString(stage_str);
                if (stage_opt.HasValue() && !entry_str.IsEmpty())
                {
                    entries.Push({
                        .stage = *stage_opt,
                        .entrypoint = entry_str,
                    });
                }
                else
                {
                    ConsoleLog(ELogLevel::Warning, "EditorShaderCompiler: Invalid #pragma se_shader: {}", line);
                }
            }
        }

        pos = line_end + 1;
    }

    return entries;
}

/**
 * VS output과 PS input의 인터페이스 불일치를 검증합니다.
 * D3D12 백엔드에서 SDL_ShaderCross가 내부적으로 SPIR-V->HLSL->DXIL 변환 시
 * DXC 최적화가 미사용 PS 입력을 제거하여 파이프라인 생성 실패를 유발할 수 있습니다.
 */
void ValidateInterStageInterface(const Path& hlsl_path, ArrayView<const ShaderCompileOutput> outputs)
{
    // VS/PS 출력 찾기
    const ShaderCompileOutput* vs_output = nullptr;
    const ShaderCompileOutput* ps_output = nullptr;
    for (const ShaderCompileOutput& output : outputs)
    {
        if (output.output_stem.EndsWith(".vert")) { vs_output = &output; }
        if (output.output_stem.EndsWith(".frag")) { ps_output = &output; }
    }

    if (!vs_output || !ps_output)
    {
        return;
    }

    // VS output 리플렉션
    SDL_ShaderCross_GraphicsShaderMetadata* vs_refl =
        SDL_ShaderCross_ReflectGraphicsSPIRV(vs_output->spirv_bytecode.Data(), vs_output->spirv_bytecode.Len(), 0);

    SDL_ShaderCross_GraphicsShaderMetadata* ps_refl =
        SDL_ShaderCross_ReflectGraphicsSPIRV(ps_output->spirv_bytecode.Data(), ps_output->spirv_bytecode.Len(), 0);

    SE_SCOPE_DEFER{
        if (vs_refl) { SDL_free(vs_refl); }
        if (ps_refl) { SDL_free(ps_refl); }
    };

    if (!vs_refl || !ps_refl)
    {
        return;
    }

    // PS input에 없는 VS output location이 있으면 D3D12에서 문제가 될 수 있음
    for (uint32 vs_i = 0; vs_i < vs_refl->num_outputs; ++vs_i)
    {
        const uint32 vs_loc = vs_refl->outputs[vs_i].location;
        bool found_in_ps = false;
        for (uint32 ps_i = 0; ps_i < ps_refl->num_inputs; ++ps_i)
        {
            if (ps_refl->inputs[ps_i].location == vs_loc)
            {
                found_in_ps = true;
                break;
            }
        }

        if (!found_in_ps)
        {
            ConsoleLog(ELogLevel::Warning,
                "EditorShaderCompiler: [{}] VS output '{}' (location {}) is not consumed by PS. "
                "This may cause D3D12 pipeline creation failure due to DXC inter-stage signature optimization.",
                hlsl_path, vs_refl->outputs[vs_i].name, vs_loc
            );
            SE_BREAKPOINT();
        }
    }
}
} // namespace

void EditorShaderCompiler::CompileAll(const Path& hlsl_dir, const Path& output_dir)
{
    FileSystem::CreateDirectories(output_dir);

    // 우선 1-depth만 순회하고, 서브디렉토리의 셰이더는 포함하지 않음.
    for (const DirectoryEntry& entry : FileSystem::ReadDir(hlsl_dir))
    {
        const Path& file_path = entry.GetPath();
        const auto ext_opt = file_path.Extension();

        if (entry.IsDirectory() || !ext_opt || *ext_opt != ".hlsl")
        {
            continue;
        }

        auto result = CompileShader(file_path);
        if (!result.HasValue())
        {
            ConsoleLog(ELogLevel::Error, "EditorShaderCompiler: {}", result.Error().What());
            continue;
        }

        for (const ShaderCompileOutput& output : result.Value())
        {
            const Path spv_path = output_dir / (output.output_stem + ".spv");
            if (!FileSystem::Write(spv_path, output.spirv_bytecode))
            {
                ConsoleLog(ELogLevel::Error, "EditorShaderCompiler: Failed to write {}", spv_path);
                continue;
            }

            ConsoleLog(ELogLevel::Info, "EditorShaderCompiler: Compiled {} -> {}", file_path, spv_path);
        }
    }
}

ShaderCompileResult<Array<ShaderCompileOutput>> EditorShaderCompiler::CompileShader(const Path& hlsl_path)
{
    // 소스를 읽어 pragma를 파싱
    FileResult<String> source_result = FileSystem::ReadToString(hlsl_path);
    if (!source_result.HasValue())
    {
        return Unexpected<ShaderCompileError>{
            ShaderCompileError::ReadFailed,
            String::Format("Failed to read shader file: {}", hlsl_path),
            hlsl_path
        };
    }

    Array<ShaderEntryPoint> pragmas = ParseShaderPragmas(*source_result);

    // FileStem: "Default.vert.hlsl" -> "Default.vert", "Default.hlsl" -> "Default"
    const auto stem_opt = hlsl_path.FileStem();
    if (!stem_opt.HasValue())
    {
        return Unexpected<ShaderCompileError>{
            ShaderCompileError::ReadFailed,
            String::Format("Failed to get file stem: {}", hlsl_path),
            hlsl_path
        };
    }
    const String& file_stem = *stem_opt;

    // pragma가 없으면 에러
    if (pragmas.IsEmpty())
    {
        return Unexpected<ShaderCompileError>{
            ShaderCompileError::NoPragma,
            String::Format("No #pragma se_shader found in shader: {}", hlsl_path),
            hlsl_path
        };
    }

    // hlsl 파일이 위치한 디렉토리를 include 경로로 사용 (예: #include "Default.hlsli" 해석)
    const Optional<Path> include_dir = hlsl_path.Parent();

    // pragma 기반 멀티 엔트리포인트 컴파일
    Array<ShaderCompileOutput> outputs;
    for (const ShaderEntryPoint& ep : pragmas)
    {
        auto spirv_bytecode = CompileHLSLToSPIRV(hlsl_path, ep.entrypoint, ep.stage, include_dir);
        if (spirv_bytecode.HasError())
        {
            return Unexpected{ std::move(spirv_bytecode).Error() };
        }

        // stem이 이미 stage 확장자로 끝나면 재추가 방지
        // "Default.vert.hlsl" -> stem="Default.vert" -> output="Default.vert" (중복 방지)
        // "Default.hlsl"      -> stem="Default"      -> output="Default.vert" / "Default.frag"
        const StringView stage_ext = StageToExtension(ep.stage);
        const String expected_suffix = String::Format(".{}", stage_ext);
        String output_stem = file_stem.EndsWith(expected_suffix)
            ? String(file_stem)
            : String::Format("{}.{}", file_stem, stage_ext);

        outputs.Push({
            .output_stem = std::move(output_stem),
            .spirv_bytecode = std::move(spirv_bytecode).Value(),
        });
    }

    // VS output / PS input 인터페이스 불일치 검증
    // SDL_ShaderCross 내부의 SPIR-V->DXIL 변환 시 DXC가 미사용 PS 입력을
    // DXIL 서명에서 제거하여 D3D12 파이프라인 생성 실패를 유발할 수 있음
    ValidateInterStageInterface(hlsl_path, outputs);

    return outputs;
}
} // namespace se::editor
