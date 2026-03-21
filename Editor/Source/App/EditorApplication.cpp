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
#include "SimpleEngine/Graphics/Scene/CollectDrawData.h"
#include "SimpleEngine/Graphics/View/FramePacket.h"
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

    const auto [world_subsystem, ui_subsystem, viewport_subsystem] =
        se::GetSubsystemsChecked<const WorldSubsystem, const EditorUISubsystem, const EditorViewportSubsystem>();

    // 1. FramePacket 조립 + 렌더 패스 추가 (단일 루프)
    se::graphics::FramePacket frame_packet;
    frame_packet.scene_draw_data = se::graphics::CollectDrawData(*world_subsystem.GetWorld());

    se::graphics::RenderGraph& graph = render_subsystem->GetRenderGraph();
    const se::graphics::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();

    for (const auto& [viewport_id, info] : viewport_subsystem.GetActiveViewportInfo())
    {
        if (ui_subsystem.GetPanel(viewport_id)->IsVisible())
        {
            if (const auto tex_resource = render_subsystem->GetRenderDevice().GetTexture(info.color_texture))
            {
                // ColorTarget Handle 생성
                const se::graphics::RGResourceHandle color_handle =
                    graph.ImportTexture(info.color_target_name, tex_resource->handle);

                // DepthTarget Handle 생성
                const se::graphics::RGResourceHandle depth_handle =
                    graph.CreateTexture(info.depth_target_name, {
                        .type = SDL_GPU_TEXTURETYPE_2D,
                        .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
                        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                        .width = info.render_view.width,
                        .height = info.render_view.height,
                        .layer_count_or_depth = 1,
                        .num_levels = 1,
                        .sample_count = SDL_GPU_SAMPLECOUNT_1,
                    });

                frame_packet.render_views.Push(info.render_view);
                graph.AddPass<se::graphics::ForwardScenePass>(
                    frame_packet.scene_draw_data, info.render_view,
                    gpu_manager, color_handle, depth_handle
                );
            }
        }
    }
    graph.AddPass<EditorUIPass>();

    // frame_packet이 살아있는 동안 실행해야 합니다 (ForwardScenePass가 const& 참조)
    render_subsystem->RenderFrame();
}
}  // namespace se::editor
