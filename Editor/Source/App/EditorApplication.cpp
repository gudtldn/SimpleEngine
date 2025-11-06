#include "App/EditorApplication.h"

#include "Rendering/EditorUIPass.h"
#include "Rendering/Compiler/Provider.h"
#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"
#include "SimpleEngine/Rendering/RenderPass/ForwardScenePass.h"
#include "SimpleEngine/Utility/Config.h"
#include "SimpleEngine/World/WorldSubsystem.h"
#include "UI/EditorUISubsystem.h"
#include "UI/EditorViewportSubsystem.h"


EditorApplication::EditorApplication()
    : Application(se::app::EApplicationMode::Editor)
{
}

void EditorApplication::RegisterSubsystems()
{
    Application::RegisterSubsystems();

    // Window 초기화
    if (PlatformSubsystem* platform_subsystem = se::utility::GetSubsystemUnchecked<PlatformSubsystem>())
    {
        using namespace se::utility;

        const VPath config_path = "Config://EngineConfig.toml";
        ParseResult result = Config::ReadConfig(config_path);
        if (!result.HasValue())
        {
            ConsoleLog(ELogLevel::Error, "Failed to read config file: {}", result.Error().description());
            return;
        }

        Config& config = result.Value();
        platform_subsystem->PrepareWindow({
            .title = config.GetValueOrStore<se::String>("window.title", "SimpleEngine Editor"),
            .width = config.GetValueOrStore<uint32>("window.width", 1280),
            .height = config.GetValueOrStore<uint32>("window.height", 720),
            .sdl_window_flags = SDL_WINDOW_RESIZABLE,
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
        using namespace se::rendering;
        using namespace se::editor::rendering;

        const RenderSubsystem* render_subsystem = se::utility::GetSubsystemUnchecked<RenderSubsystem>();
        PSOManager& pso_manager = render_subsystem->GetPSOManager();
        pso_manager.SetShaderCacheProvider<CompilingShaderProvider>();
    }

    return true;
}

void EditorApplication::Render()
{
    Application::Render();

    const RenderSubsystem* render_subsystem = se::utility::GetSubsystemUnchecked<RenderSubsystem>();
    {
        using namespace se::editor::ui;
        using namespace se::editor::rendering;

        const WorldSubsystem* world_subsystem = se::utility::GetSubsystemUnchecked<WorldSubsystem>();
        const EditorUISubsystem* ui_subsystem = se::utility::GetSubsystemUnchecked<EditorUISubsystem>();
        const EditorViewportSubsystem* viewport_subsystem = se::utility::GetSubsystemUnchecked<EditorViewportSubsystem>();

        se::world::World& world_ref = *world_subsystem->GetWorld();
        se::rendering::RenderGraph& graph = render_subsystem->GetRenderGraph();
        for (const auto& [viewport_id, info] : viewport_subsystem->GetActiveViewportInfo())
        {
            if (ui_subsystem->GetPanel(viewport_id)->IsVisible())
            {
                const StringName color_target_name = viewport_id;
                graph.ImportTexture(color_target_name, info.color_texture);

                const StringName depth_target_name = se::String::Format("{}_Depth", viewport_id.ToString());
                graph.AddPass<se::rendering::ForwardScenePass>(
                    world_ref,
                    color_target_name, depth_target_name,
                    info.width, info.height
                );
            }
        }
        graph.AddPass<EditorUIPass>();
    }
    render_subsystem->RenderFrame();
}
