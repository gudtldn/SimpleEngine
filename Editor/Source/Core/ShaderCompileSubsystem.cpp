// ReSharper disable CppMemberFunctionMayBeConst
#include "Core/ShaderCompileSubsystem.h"
#include "Graphics/EditorShaderCompiler.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#if SE_HAS_HLSL_COMPILER
#include "SDL3_shadercross/SDL_shadercross.h"
#endif


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(ShaderCompileSubsystem)
    .DependsOn<EventSubsystem>()
    .UpdateDependsOn<InputSubsystem>();

SE_BEGIN_REFLECT(ShaderCompileSubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(ShaderCompileSubsystem)

bool ShaderCompileSubsystem::Initialize()
{
#if SE_HAS_HLSL_COMPILER
    // SDL_ShaderCross_Init는 내부적으로 DXC 라이브러리를 Load 함
    if (!SDL_ShaderCross_Init())
    {
        ConsoleLog(ELogLevel::Error, "Failed to initialize SDL_ShaderCross!");
        return false;
    }

    sources.Push({ .source_dir = VFS::ToPath("CoreShader://"), .output_dir = VFS::ToPath("CoreShader://Compiled") });
    sources.Push({ .source_dir = VFS::ToPath("EditorShader://"), .output_dir = VFS::ToPath("EditorShader://Compiled") });
#endif
    return true;
}

void ShaderCompileSubsystem::Release()
{
#if SE_HAS_HLSL_COMPILER
    sources.Clear();
    pending_files.Clear();

    SDL_ShaderCross_Quit();
#endif
}

void ShaderCompileSubsystem::Update([[maybe_unused]] f64 delta_time)
{
#if SE_HAS_HLSL_COMPILER
    if (const InputSubsystem* input = se::GetSubsystem<InputSubsystem>())
    {
        if (input->IsKeyPressed(EKeyCode::F5))
        {
            RecompileChanged();
        }
    }
#endif
}

void ShaderCompileSubsystem::RecompileChanged()
{
    pending_files.Clear();

    for (const auto [i, src] : sources | std::views::enumerate)
    {
        for (const DirectoryEntry& entry : FileSystem::ReadDir(src.source_dir))
        {
            if (!entry.IsFile()) { continue; }

            const Path& hlsl_path = entry.GetPath();
            if (hlsl_path.Extension() != ".hlsl") { continue; }

            const u64 hlsl_mtime = entry.LastWriteTime();

            const auto stem_opt = hlsl_path.FileStem();
            if (!stem_opt) { continue; }

            // output_dir에서 "{stem}.*.spv" 패턴의 파일들과 mtime 비교
            bool any_spv_found = false;
            bool any_spv_outdated = false;
            const String spv_prefix = String::Format("{}.", *stem_opt);

            if (FileSystem::Exists(src.output_dir))
            {
                for (const DirectoryEntry& spv_entry : FileSystem::ReadDir(src.output_dir))
                {
                    if (!spv_entry.IsFile()) { continue; }

                    const Path& spv_path = spv_entry.GetPath();
                    if (spv_path.Extension() != ".spv") { continue; }

                    const auto spv_stem_opt = spv_path.FileStem();
                    if (!spv_stem_opt) { continue; }

                    // "WorldGrid.hlsl" -> spv_stem_opt "WorldGrid.vert" 처럼 prefix 매칭
                    // "Default.frag.hlsl" -> spv_stem_opt "Default.frag" 처럼 exact 매칭
                    if (*spv_stem_opt != *stem_opt && !spv_stem_opt->StartsWith(spv_prefix)) { continue; }

                    any_spv_found = true;
                    if (spv_entry.LastWriteTime() < hlsl_mtime)
                    {
                        any_spv_outdated = true;
                    }
                }
            }

            if (!any_spv_found || any_spv_outdated)
            {
                pending_files.Insert(hlsl_path, static_cast<usize>(i));
            }
        }
    }

    if (pending_files.IsEmpty())
    {
        ConsoleLog(ELogLevel::Info, "No shader changes detected.");
        return;
    }

    ConsoleLog(ELogLevel::Info, "Recompiling {} changed shader(s)...", pending_files.Len());
    RecompilePending();
}

void ShaderCompileSubsystem::RecompilePending()
{
    bool any_compiled = false;

    for (const auto& [hlsl_path, source_idx] : pending_files)
    {
        const ShaderSource& src = sources[source_idx];

        auto result = EditorShaderCompiler::CompileShader(hlsl_path);
        if (!result.HasValue())
        {
            ConsoleLog(ELogLevel::Error, "EditorShaderCompiler: {}", result.Error().What());
            continue;
        }

        FileSystem::CreateDirectories(src.output_dir);

        for (const ShaderCompileOutput& output : result.Value())
        {
            const Path spv_path = src.output_dir / Path(output.output_stem + ".spv");
            if (!FileSystem::Write(spv_path, output.spirv_bytecode))
            {
                ConsoleLog(ELogLevel::Error, "EditorShaderCompiler: Failed to write {}", spv_path);
                continue;
            }

            ConsoleLog(ELogLevel::Info, "EditorShaderCompiler: Compiled {} -> {}", hlsl_path, spv_path);
        }

        any_compiled = true;
    }

    if (any_compiled)
    {
        if (const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>())
        {
            // 안전한 리소스 해제를 위해 GPU가 작업을 모두 마칠 때까지 대기
            SDL_WaitForGPUIdle(render_subsystem->GetRenderDevice().GetRawDevice());
            render_subsystem->GetPSOManager().ClearAll();
            ConsoleLog(ELogLevel::Info, "Shader cache and pipelines cleared.");
        }
    }
}
} // namespace se::editor
