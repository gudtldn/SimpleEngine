#include "Core/ShaderCompileSubsystem.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(ShaderCompileSubsystem)
    .DependsOn<EventSubsystem>()
    .DependsOn<AssetSubsystem>()
    .UpdateDependsOn<InputSubsystem>();

SE_BEGIN_REFLECT_V1(ShaderCompileSubsystem, meta::Reflect, meta::Hidden, meta::Transient)
    SE_REFLECT_INTERFACE_V1(IUpdatable)
SE_END_REFLECT_V1(ShaderCompileSubsystem)

namespace
{
// TODO: 셰이더 폴더가 하드코딩 되어있음. 추후 Config에서 불러오도록 변경
/** 쿡할 셰이더 폴더 */
const VPath SHADER_DIRECTORIES[] = { VPath{ "CoreShader://" }, VPath{ "EditorShader://" } };
} // namespace

bool ShaderCompileSubsystem::Initialize()
{
#if SE_HAS_HLSL_COMPILER
    compiler = std::make_unique<ShaderCompiler>();
    cooker = std::make_unique<ShaderCooker>(*compiler);

    // 셰이더는 첫 그리기 때 DDC에서 읽으므로, 첫 프레임 전에 쿡을 끝냅니다.
    (void)CookAll();
#endif
    return true;
}

void ShaderCompileSubsystem::Release()
{
    cooker.reset();
    compiler.reset();
}

void ShaderCompileSubsystem::Update([[maybe_unused]] f64 delta_time)
{
#if SE_HAS_HLSL_COMPILER
    if (const InputSubsystem* input = se::GetSubsystem<InputSubsystem>())
    {
        if (input->IsKeyPressed(EKeyCode::F5))
        {
            RecookChanged();
        }
    }
#endif
}

u32 ShaderCompileSubsystem::CookAll() const
{
    DerivedDataCache& ddc = se::GetSubsystemChecked<AssetSubsystem>().GetDDC();

    u32 cooked = 0;
    for (const VPath& shader_dir : SHADER_DIRECTORIES)
    {
        const ShaderCookSummary summary = cooker->CookDirectory(shader_dir, ddc);
        ConsoleLog(
            ELogLevel::Info,
            "Shader cook {}: {} cooked, {} up to date, {} failed",
            shader_dir.ToString(), summary.cooked, summary.up_to_date, summary.failed
        );
        cooked += summary.cooked;
    }
    return cooked;
}

void ShaderCompileSubsystem::RecookChanged() const
{
    if (CookAll() == 0)
    {
        ConsoleLog(ELogLevel::Info, "No shader changes detected.");
        return;
    }

    // 쿡에 실패한 셰이더는 DDC에 이전 번들이 남아 있으므로, 비운 뒤 다시 읽어도 이전 셰이더가 사용됨
    if (const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>())
    {
        // 안전한 리소스 해제를 위해 GPU가 작업을 모두 마칠 때까지 대기
        SDL_WaitForGPUIdle(render_subsystem->GetRenderDevice().GetRawDevice());
        render_subsystem->GetPSOManager().ClearAll();
        ConsoleLog(ELogLevel::Info, "Shader cache and pipelines cleared.");
    }
}
} // namespace se::editor
