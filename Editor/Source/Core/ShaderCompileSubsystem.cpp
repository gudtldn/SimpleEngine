// ReSharper disable CppMemberFunctionMayBeConst
#include "Core/ShaderCompileSubsystem.h"
#include "SimpleEditor/Core/FileWatcherSubsystem.h"
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
    .DependsOn<EventSubsystem, FileWatcherSubsystem>()
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

    sources.Push({ VFS::ToPath("CoreShader://"), VFS::ToPath("CoreShader://Compiled"), {} });
    sources.Push({ VFS::ToPath("EditorShader://"), VFS::ToPath("EditorShader://Compiled"), {} });

    if (FileWatcherSubsystem* fw = se::GetSubsystem<FileWatcherSubsystem>())
    {
        for (ShaderSource& src : sources)
        {
            src.watch_id = fw->Watch(src.source_dir);
        }
    }
#endif
    return true;
}

void ShaderCompileSubsystem::Release()
{
#if SE_HAS_HLSL_COMPILER
    if (FileWatcherSubsystem* fw = se::GetSubsystem<FileWatcherSubsystem>())
    {
        for (const ShaderSource& src : sources)
        {
            if (src.watch_id)
            {
                fw->Unwatch(src.watch_id);
            }
        }
    }
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
            RecompileAll();
            return;
        }
    }

    FileWatcherSubsystem* fw = se::GetSubsystem<FileWatcherSubsystem>();
    if (!fw)
    {
        return;
    }

    pending_files.Clear(); // 노드 재사용
    for (usize i = 0; i < sources.Len(); ++i)
    {
        for (const FileWatchEvent& event : fw->DrainEvents(sources[i].watch_id))
        {
            // hlsl만 분류
            if (event.filename.EndsWith(".hlsl"))
            {
                pending_files.Insert(event.directory / event.filename, i);
            }
        }
    }

    if (!pending_files.IsEmpty())
    {
        RecompilePending();
    }
#endif
}

void ShaderCompileSubsystem::RecompileAll()
{
    ConsoleLog(ELogLevel::Info, "Recompiling shaders...");

    for (const ShaderSource& src : sources)
    {
        EditorShaderCompiler::CompileAll(src.source_dir, src.output_dir);
    }

    if (const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>())
    {
        // 안전한 리소스 해제를 위해 GPU가 작업을 모두 마칠 때까지 대기
        SDL_WaitForGPUIdle(render_subsystem->GetRenderDevice().GetRawDevice());
        render_subsystem->GetPSOManager().ClearAll();
        ConsoleLog(ELogLevel::Info, "Shader cache and pipelines cleared.");
    }
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
