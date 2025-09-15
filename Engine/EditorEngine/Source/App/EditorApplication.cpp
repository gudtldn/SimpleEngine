module SE.Editor.App;

import SE.Editor.Rendering;
import SE.Editor.Subsystems.ShaderCompileSubsystem;
import SE.Editor.Subsystems.ImGuiSubsystem;

import SE.Core;
import SE.Types;
import SE.Config;
import SE.Subsystems.PlatformSubsystem;
import SE.Subsystems.RenderSubsystem;

import std;
import <SDL3/SDL_gpu.h>;


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
        using namespace se::config;

        const std::filesystem::path solution_path = std::filesystem::current_path().parent_path().parent_path();
        const std::filesystem::path config_path = solution_path / u8"Config/EngineConfig.toml";

        ParseResult result = Config::ReadConfig(config_path);
        if (!result.has_value())
        {
            ConsoleLog(ELogLevel::Error, u8"Failed to read config file: {}", result.error().description());
            return;
        }

        Config& config = result.value();
        platform_subsystem->PrepareWindow({
            .title = config.GetValueOrStore<se::u8string>(u8"window.title", u8"SimpleEngine Editor"),
            .width = config.GetValueOrStore<uint32>(u8"window.width", 1280),
            .height = config.GetValueOrStore<uint32>(u8"window.height", 720),
            .sdl_window_flags = SDL_WINDOW_RESIZABLE,
            .swapchain_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            .present_mode = SDL_GPU_PRESENTMODE_MAILBOX,
        });

        if (!config.WriteConfig(config_path))
        {
            ConsoleLog(ELogLevel::Error, u8"Failed to write config file: {}", config_path.generic_u8string());
            return;
        }
    }

    // GPU Device 초기화
    engine_instance->RegisterSubsystem<RenderSubsystem>();

    // ImGui 초기화
    engine_instance->RegisterSubsystem<ImGuiSubsystem>();

    // SDL_shadercross 초기화
    engine_instance->RegisterSubsystem<ShaderCompileSubsystem>();
}

bool EditorApplication::PostInitialize()
{
    if (!Application::PostInitialize())
    {
        return false;
    }

    // Shader Cache Provider 변경
    {
        using namespace se::rendering::manager;
        using namespace se::editor::rendering::shader_provider;

        const RenderSubsystem* render_subsystem = engine_instance->GetSubsystem<RenderSubsystem>();
        PSOManager* pso_manager = render_subsystem->GetPSOManager();
        pso_manager->SetShaderCacheProvider<CompilingShaderProvider>();
    }

    return true;
}

void EditorApplication::Render()
{
    Application::Render();

    RenderSubsystem* render_subsystem = engine_instance->GetSubsystem<RenderSubsystem>();
    {
        using namespace se::editor::rendering::passes;
        render_subsystem->GetRenderGraph()->AddPass<EditorUIPass>();
    }
    render_subsystem->RenderFrame();
}
