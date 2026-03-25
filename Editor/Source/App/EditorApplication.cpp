#include "SimpleEditor/App/EditorApplication.h"

#include "Core/Logging/Backend/EditorConsoleBackend.h"
#include "Graphics/EditorUIPass.h"
#include "Graphics/Compiler/Provider.h"
#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEditor/UI/EditorUISubsystem.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Asset/AssetPool.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
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

    // FramePacket 조립 (SceneDrawData 수집)
    se::graphics::FramePacket frame_packet;
    frame_packet.scene_draw_data = se::graphics::CollectDrawData(*world_subsystem.GetWorld());

    // GPU 메모리에 올라오지 않은 메시를 렌더링 전에 업로드
    EnsureMeshesResident(frame_packet.scene_draw_data);

    // TODO: GPU 리소스 해제 로직
    // - 현재 Bump Pointer 할당이라 개별 VRAM 회수 불가 (UnloadMesh는 매핑만 제거)
    // - Defragmentation 구현 후: ECS에서 참조되지 않는 mesh_id를 주기적으로 스캔하여 UnloadMesh() 호출
    // - 씬 전환 시: GpuResourceManager 전체 리셋 고려

    const se::graphics::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();

    // RenderFrame 람다 내에서 패스 조립
    // back_buffer_handle은 RenderSubsystem이 스왑체인을 취득한 후 전달
    render_subsystem->RenderFrame(
        [&](se::graphics::RGTextureHandle swapchain_handle, se::graphics::RenderGraphBuilder& builder)
        {
            for (const auto& [viewport_id, info] : viewport_subsystem.GetActiveViewportInfo())
            {
                if (ui_subsystem.GetPanel(viewport_id)->IsVisible())
                {
                    if (const auto tex_resource = render_subsystem->GetRenderDevice().GetTexture(info.color_texture))
                    {
                        // ColorTarget Handle 생성
                        const se::graphics::RGTextureHandle color_handle =
                            builder.ImportTexture(info.color_target_name, tex_resource->handle);

                        // DepthTarget Handle 생성
                        const se::graphics::RGTextureHandle depth_handle =
                            builder.CreateTexture(info.depth_target_name, {
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
                        builder.AddPass<se::graphics::ForwardScenePass>(
                            frame_packet.scene_draw_data, info.render_view,
                            gpu_manager, color_handle, depth_handle
                        );
                    }
                }
            }

            // BackBuffer 핸들을 생성자 DI로 전달
            builder.AddPass<EditorUIPass>(swapchain_handle);
        }
    );
}

void EditorApplication::EnsureMeshesResident(const graphics::SceneDrawData& in_scene_data)
{
    if (in_scene_data.opaque_commands.IsEmpty())
    {
        return;
    }

    const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>();
    auto* asset_subsystem = se::GetSubsystem<asset::AssetSubsystem>();
    if (!render_subsystem || !asset_subsystem)
    {
        return;
    }

    graphics::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();
    const asset::AssetRegistry& registry = asset_subsystem->GetRegistry();

    // 업로드가 필요한 메시를 수집 (아직 업로드되지 않은 데이터 또는 cook_key 변경)
    struct PendingUpload
    {
        asset::AssetPath path;
        String cook_key;
    };
    HashMap<asset::AssetId, PendingUpload> pending;

    for (const auto& cmd : in_scene_data.opaque_commands)
    {
        // 이미 pending에 있으면 건너뜀
        if (pending.Contains(cmd.mesh_id))
        {
            continue;
        }

        // Registry에서 경로와 cook_key를 조회
        asset::AssetPath asset_path;
        String cook_key;
        if (!registry.ReadRecord(cmd.mesh_id, [&](const asset::AssetRecord& record)
        {
            asset_path = record.logical_path;
            cook_key = String::Format("{}|{}", record.metadata.source_hash, record.metadata.settings_hash);
        }))
        {
            continue;
        }

        if (gpu_manager.GetSlice(cmd.mesh_id).HasValue())
        {
            // GPU 메모리에 이미 올라와 있음 (cook_key가 동일하면 최신 상태)
            const Optional<String&> prev_key = uploaded_mesh_hashes.Find(cmd.mesh_id);
            if (prev_key == cook_key)
            {
                continue;
            }

            // cook_key 불일치 -> 에셋이 reimport됨 (Pool과 GPU를 무효화)
            gpu_manager.UnloadMesh(cmd.mesh_id);
            asset_subsystem->GetPool().Remove(cmd.mesh_id);
            ConsoleLog(ELogLevel::Info, "GPU mesh invalidated (reimport detected): {}", asset_path.ToString());
        }

        pending.Insert(cmd.mesh_id, {
            .path = std::move(asset_path),
            .cook_key = std::move(cook_key),
        });
    }

    if (pending.IsEmpty())
    {
        return;
    }

    // 업로드용 Command Buffer 생성
    graphics::RenderDevice& device = render_subsystem->GetRenderDevice();
    SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(device.GetRawDevice());
    if (!upload_cmd)
    {
        return;
    }

    // 각 메시를 Registry에서 경로 조회 -> Load -> GPU 업로드
    for (const auto& [mesh_id, info] : pending)
    {
        // Load를 호출하여 DDC에서 역직렬화 -> Pool에 등록
        asset::AssetHandle<asset::StaticMesh> handle = asset_subsystem->Load<asset::StaticMesh>(info.path);
        if (!handle)
        {
            ConsoleLog(ELogLevel::Warning, "EnsureMeshesResident: Load failed for {}", info.path.ToString());
            continue;
        }

        const asset::StaticMesh* mesh = handle.Get();
        if (!mesh || mesh->vertices.IsEmpty())
        {
            ConsoleLog(ELogLevel::Warning, "EnsureMeshesResident: Empty mesh data for {}", info.path.ToString());
            continue;
        }

        const bool success = gpu_manager.UploadMesh(
            upload_cmd,
            mesh_id,
            mesh->vertices.Data(),
            static_cast<uint32>(mesh->vertices.Len() * sizeof(graphics::Vertex)),
            mesh->indices.Data(),
            static_cast<uint32>(mesh->indices.Len() * sizeof(uint32))
        );

        if (success)
        {
            uploaded_mesh_hashes.Insert(mesh_id, info.cook_key);
        }
    }

    SDL_SubmitGPUCommandBuffer(upload_cmd);
}
} // namespace se::editor
