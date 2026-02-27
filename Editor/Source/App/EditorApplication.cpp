#include "SimpleEditor/App/EditorApplication.h"

#include "Core/Logging/Backend/EditorConsoleBackend.h"
#include "Graphics/EditorUIPass.h"
#include "Graphics/Compiler/Provider.h"
#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEditor/UI/EditorUISubsystem.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Graphics/RenderPass/ForwardScenePass.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se::editor
{
EditorApplication::EditorApplication()
    : Application(se::EApplicationMode::Editor)
{
}

void EditorApplication::Startup(const String& cmd_line)
{
    {
        LogBackendManager& manager = LogBackendManager::Get();

        manager.AddBackend<EditorConsoleBackend>();
    }

    Application::Startup(cmd_line);
}

void EditorApplication::RegisterSubsystems()
{
    Application::RegisterSubsystems();

    // Window 초기화
    if (WindowSubsystem* window_subsystem = se::GetSubsystem<WindowSubsystem>())
    {
        using namespace se;

        const VPath config_path = "Config://EngineConfig.toml";

        // ConfigFile로 설정 로드 (파일이 없으면 기본값 사용)
        WindowSettings window_settings;
        GraphicsSettings graphics_settings;
        PerformanceSettings performance_settings;

        if (auto result = ConfigFile::Load(config_path))
        {
            const ConfigFile& config = result.Value();
            window_settings = config.GetSection<WindowSettings>("window");
            graphics_settings = config.GetSection<GraphicsSettings>("graphics");
            performance_settings = config.GetSection<PerformanceSettings>("performance");
        }
        else
        {
            ConsoleLog(ELogLevel::Warning, "Config file not found, using defaults: {}", result.Error());
        }

        // Performance 설정 즉시 적용
        SetTargetFps(performance_settings.target_fps);
        SetBusyWaitRatio(performance_settings.busy_wait_ratio);

        uint32 flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (window_settings.fullscreen) { flags |= SDL_WINDOW_FULLSCREEN; }
        if (window_settings.borderless) { flags |= SDL_WINDOW_BORDERLESS; }
        if (window_settings.resizable)  { flags |= SDL_WINDOW_RESIZABLE;  }

        // Present Mode 변환
        SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
        switch (graphics_settings.present_mode)
        {
        case EPresentMode::VSync:
            present_mode = SDL_GPU_PRESENTMODE_VSYNC;
            break;
        case EPresentMode::Immediate:
            present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
            break;
        case EPresentMode::Mailbox:
        default:
            break;
        }

        window_subsystem->PrepareWindow({
            .title = window_settings.title,
            .width = window_settings.width,
            .height = window_settings.height,
            .sdl_window_flags = flags,
            .swapchain_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            .present_mode = present_mode,
        });
    }
}

bool EditorApplication::PostInitialize()
{
    if (!Application::PostInitialize())
    {
        return false;
    }

    // Shader Cache Provider 변경
    {
        using namespace se::graphics;
        using namespace se::editor;

        if (const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>())
        {
            PSOManager& pso_manager = render_subsystem->GetPSOManager();
            pso_manager.SetShaderCacheProvider<CompilingShaderProvider>();
        }
    }

    return true;
}

void EditorApplication::Render()
{
    Application::Render();

    const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>();
    if (!render_subsystem)
    {
        return;
    }

    {
        const auto [world_subsystem, ui_subsystem, viewport_subsystem] =
            se::GetSubsystemsChecked<const WorldSubsystem, const EditorUISubsystem, const EditorViewportSubsystem>();

        se::ecs::World& world_ref = *world_subsystem.GetWorld();
        se::graphics::RenderGraph& graph = render_subsystem->GetRenderGraph();
        for (const auto& [viewport_id, info] : viewport_subsystem.GetActiveViewportInfo())
        {
            if (ui_subsystem.GetPanel(viewport_id)->IsVisible())
            {
                const StringName color_target_name = viewport_id;
                graph.ImportTexture(color_target_name, info.color_texture);

                // TODO: 나중에 에디터 카메라나, 월드 카메라 분기 처리
                const Matrix4x4 vp_matrix_to_render = info.view_matrix * info.projection_matrix;

                const StringName depth_target_name = se::String::Format("{}_Depth", viewport_id.ToString());
                graph.AddPass<se::graphics::ForwardScenePass>(
                    world_ref,
                    vp_matrix_to_render,
                    color_target_name, depth_target_name,
                    info.width, info.height
                );
            }
        }
        graph.AddPass<EditorUIPass>();
    }
    render_subsystem->RenderFrame();
}
}  // namespace se::editor
