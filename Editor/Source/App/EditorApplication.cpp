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
#include "UI/Panels/ViewportPanel.h"


EditorApplication::EditorApplication()
    : Application(se::app::EApplicationMode::Editor)
{
}

void EditorApplication::RegisterSubsystems()
{
    Application::RegisterSubsystems();

    // Window 초기화
    if (PlatformSubsystem* platform_subsystem = engine_instance->GetSubsystem<PlatformSubsystem>())
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

        const RenderSubsystem* render_subsystem = engine_instance->GetSubsystem<RenderSubsystem>();
        PSOManager& pso_manager = render_subsystem->GetPSOManager();
        pso_manager.SetShaderCacheProvider<CompilingShaderProvider>();
    }

    return true;
}

void EditorApplication::Render()
{
    Application::Render();

    const RenderSubsystem* render_subsystem = engine_instance->GetSubsystem<RenderSubsystem>();
    {
        using namespace se::editor::ui;
        using namespace se::editor::rendering;

        const WorldSubsystem* world_subsystem = engine_instance->GetSubsystem<WorldSubsystem>();
        const EditorUISubsystem* editor_ui_subsystem = engine_instance->GetSubsystem<EditorUISubsystem>();

        // 임시로 화면의 크기를 인자로 넣어줌
        const ViewportPanel& viewport_panel = *editor_ui_subsystem->GetPanel<ViewportPanel>();
        const uint32 width = viewport_panel.GetViewportWidth();
        const uint32 height = viewport_panel.GetViewportHeight();

        se::rendering::RenderGraph& graph = render_subsystem->GetRenderGraph();

        graph.ImportTexture(se::rendering::ForwardScenePass::SceneColorTarget, viewport_panel.GetViewportColorTexture());
        graph.ImportTexture(se::rendering::ForwardScenePass::SceneDepthTarget, viewport_panel.GetViewportDepthTexture());

        graph.AddPass<se::rendering::ForwardScenePass>(
            *world_subsystem->GetWorld(), width, height
        );
        graph.AddPass<EditorUIPass>();
    }
    render_subsystem->RenderFrame();
}
