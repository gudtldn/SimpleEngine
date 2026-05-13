#include "Core/ShaderCompileSubsystem.h"
#include "Graphics/EditorShaderCompiler.h"

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
#endif
    return true;
}

void ShaderCompileSubsystem::Release()
{
#if SE_HAS_HLSL_COMPILER
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
            ConsoleLog(ELogLevel::Info, "F5 Pressed: Recompiling shaders...");

            // TODO: 현재 EditorApplication과 로직이 중복됨. 추후 FileWatcher 도입 시 셰이더 관리 시스템으로 통합 예정
            const Path hlsl_dir = VFS::ToPath("CoreShader://");
            const Path output_dir = VFS::ToPath("CoreShader://Compiled");
            EditorShaderCompiler::CompileAll(hlsl_dir, output_dir);

            const Path editor_hlsl_dir = VFS::ToPath("EditorShader://");
            const Path editor_output_dir = VFS::ToPath("EditorShader://Compiled");
            EditorShaderCompiler::CompileAll(editor_hlsl_dir, editor_output_dir);

            if (const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>())
            {
                // 안전한 리소스 해제를 위해 GPU가 작업을 모두 마칠 때까지 대기
                SDL_WaitForGPUIdle(render_subsystem->GetRenderDevice().GetRawDevice());
                render_subsystem->GetPSOManager().ClearAll();
                ConsoleLog(ELogLevel::Info, "Shader cache and pipelines cleared.");
            }
        }
    }
#endif
}
} // namespace se::editor
