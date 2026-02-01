#include "App/EditorApplication.h"

#include "Core/Logging/Backend/EditorConsoleBackend.h"
#include "Graphics/EditorUIPass.h"
#include "Graphics/Compiler/Provider.h"
#include "UI/EditorUISubsystem.h"
#include "UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Graphics/RenderPass/ForwardScenePass.h"
#include "SimpleEngine/Utility/Config.h"
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
    if (PlatformSubsystem* platform_subsystem = se::GetSubsystem<PlatformSubsystem>())
    {
        using namespace se;

        const VPath config_path = "Config://EngineConfig.toml";
        ParseResult result = Config::ReadConfig(config_path);
        if (!result.HasValue())
        {
            ConsoleLog(ELogLevel::Error, "Failed to read config file: {}", result.Error().description());
            return;
        }

        Config& config = result.Value();
        uint32 flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;

        if (config.GetValueOrStore<bool>("window.fullscreen", false))
        {
            flags |= SDL_WINDOW_FULLSCREEN;
        }

        if (config.GetValueOrStore<bool>("window.borderless", false))
        {
            flags |= SDL_WINDOW_BORDERLESS;
        }

        if (config.GetValueOrStore<bool>("window.resizable", true))
        {
            flags |= SDL_WINDOW_RESIZABLE;
        }

        platform_subsystem->PrepareWindow({
            .title = config.GetValueOrStore<String>("window.title", "SimpleEngine Editor"),
            .width = config.GetValueOrStore<uint32>("window.width", 1280),
            .height = config.GetValueOrStore<uint32>("window.height", 720),
            .sdl_window_flags = flags,
            .swapchain_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            .present_mode = SDL_GPU_PRESENTMODE_MAILBOX,
        });

        if (!config.WriteConfig(config_path))
        {
            ConsoleLog(ELogLevel::Error, "Failed to write config file: {}", config_path.ToString());
            return;
        }
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
        using namespace se::editor;
        using namespace se::editor;

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
