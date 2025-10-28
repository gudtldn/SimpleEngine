#include "App/EditorApplication.h"

#include "Rendering/EditorUIPass.h"
#include "Rendering/Compiler/Provider.h"
#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"
#include "SimpleEngine/Rendering/RenderPass/ForwardScenePass.h"
#include "SimpleEngine/Utility/Config.h"
#include "SimpleEngine/World/WorldSubsystem.h"


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

        const VPath config_path = u8"Config://EngineConfig.toml";
        ParseResult result = Config::ReadConfig(config_path);
        if (!result.has_value())
        {
            ConsoleLog(ELogLevel::Error, "Failed to read config file: {}", result.error().description());
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
        using namespace se::editor::rendering;
        const PlatformSubsystem* platform_subsystem = engine_instance->GetSubsystem<PlatformSubsystem>();
        const WorldSubsystem* world_subsystem = engine_instance->GetSubsystem<WorldSubsystem>();

        // 임시로 화면의 크기를 인자로 넣어줌
        int32 width, height;
        SDL_GetWindowSize(platform_subsystem->GetMainWindow(), &width, &height);

        render_subsystem->GetRenderGraph().AddPass<se::rendering::ForwardScenePass>(
            *world_subsystem->GetWorld(),
            static_cast<uint32>(width), static_cast<uint32>(height)
        );
        render_subsystem->GetRenderGraph().AddPass<EditorUIPass>(
            static_cast<uint32>(width), static_cast<uint32>(height)
        );
    }
    render_subsystem->RenderFrame();
}
