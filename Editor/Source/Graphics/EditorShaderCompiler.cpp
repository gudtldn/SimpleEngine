#include "Graphics/EditorShaderCompiler.h"
#include "Graphics/Compiler/Compiler.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/ShaderUtils.h"


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
} // namespace

void EditorShaderCompiler::CompileAll(const Path& hlsl_dir, const Path& output_dir)
{
    FileSystem::CreateDirectories(output_dir);

    // 우선 1-depth만 순회하고, 서브디렉토리의 셰이더는 포함하지 않음.
    for (const DirectoryEntry& entry : FileSystem::ReadDir(hlsl_dir))
    {
        const Path& file_path = entry.GetPath();
        const auto ext_opt = file_path.Extension();

        if (!ext_opt || !ext_opt->Contains(".hlsl"))
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
        return Unexpected{
            ShaderCompileError(
                ShaderCompileError::ECode::ReadFailed,
                String::Format("Failed to read shader file: {}", hlsl_path),
                hlsl_path
            )
        };
    }

    Array<ShaderEntryPoint> pragmas = ParseShaderPragmas(*source_result);

    // FileStem: "Default.vert.hlsl" -> "Default.vert", "Default.hlsl" -> "Default"
    const auto stem_opt = hlsl_path.FileStem();
    if (!stem_opt.HasValue())
    {
        return Unexpected{
            ShaderCompileError(
                ShaderCompileError::ECode::ReadFailed,
                String::Format("Failed to get file stem: {}", hlsl_path),
                hlsl_path
            )
        };
    }
    const String& file_stem = *stem_opt;

    // pragma가 없으면 에러
    if (pragmas.IsEmpty())
    {
        return Unexpected{
            ShaderCompileError(
                ShaderCompileError::ECode::NoPragma,
                String::Format("No #pragma se_shader found in shader: {}", hlsl_path),
                hlsl_path
            )
        };
    }

    // pragma 기반 멀티 엔트리포인트 컴파일
    Array<ShaderCompileOutput> outputs;
    for (const ShaderEntryPoint& ep : pragmas)
    {
        auto spirv_bytecode = CompileHLSLToSPIRV(hlsl_path, ep.entrypoint, ep.stage);
        if (spirv_bytecode.HasError())
        {
            return Unexpected{ std::move(spirv_bytecode).Error() };
        }

        // "Default.hlsl" -> stem="Default" -> "Default.vert" / "Default.frag"
        String output_stem = String::Format("{}.{}", file_stem, StageToExtension(ep.stage));
        outputs.Push({
            .output_stem = std::move(output_stem),
            .spirv_bytecode = std::move(spirv_bytecode).Value(),
        });
    }

    return outputs;
}
} // namespace se::editor
