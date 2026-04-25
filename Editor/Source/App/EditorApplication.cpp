#include "SimpleEditor/App/EditorApplication.h"

#include "Core/Logging/Backend/EditorConsoleBackend.h"
#include "Graphics/EditorShaderCompiler.h"
#include "Graphics/EditorUIPass.h"
#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEditor/Gizmo/GizmoPass.h"
#include "SimpleEditor/Gizmo/GizmoPickPass.h"
#include "SimpleEditor/Gizmo/GizmoSubsystem.h"
#include "SimpleEditor/Picking/PickSubsystem.h"
#include "SimpleEditor/UI/EditorUISubsystem.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"
#include "SimpleEditor/WorldGrid/WorldGridPass.h"

#include "SimpleEngine/Asset/AssetPool.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Debug/DebugDrawSubsystem.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Graphics/RenderPass/DebugLinePass.h"
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

    // TODO: 여기 하드코딩 되어있음. 추후 Config에서 불러와서 사용하던가 하는 방향으로
    // 초기 셰이더 컴파일: 모든 .hlsl -> .spv (DXC를 사용할 수 없는 플랫폼에서는 미리 구워진 .spv만 사용)
#if SE_HAS_HLSL_COMPILER
    {
        // TODO: 현재 ShaderCompileSubsystem과 로직이 중복됨. 추후 FileWatcher 도입 시 셰이더 관리 시스템으로 통합 예정
        const Path hlsl_dir = VFS::ToPath("CoreShader://");
        const Path output_dir = VFS::ToPath("CoreShader://Compiled");

        EditorShaderCompiler::CompileAll(hlsl_dir, output_dir);
    }
    {
        const Path editor_hlsl_dir = VFS::ToPath("EditorShader://");
        const Path editor_output_dir = VFS::ToPath("EditorShader://Compiled");

        EditorShaderCompiler::CompileAll(editor_hlsl_dir, editor_output_dir);
    }
#endif

    return true;
}

void EditorApplication::Render()
{
    Application::Render();

    // 기즈모 업데이트: Clear + 선택된 엔티티에 대한 기즈모 렌더링
    GizmoSubsystem* gizmo_subsystem = se::GetSubsystem<GizmoSubsystem>();
    if (gizmo_subsystem)
    {
        gizmo_subsystem->DrawGizmos();
    }

    PickSubsystem* pick_subsystem = se::GetSubsystem<PickSubsystem>();

    const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>();
    if (!render_subsystem)
    {
        return;
    }

    const auto [entity_subsystem, ui_subsystem, viewport_subsystem] =
        se::GetSubsystemsChecked<const EntitySubsystem, const EditorUISubsystem, const EditorViewportSubsystem>();

    // FramePacket 조립 (SceneDrawData 수집)
    se::graphics::FramePacket frame_packet;
    frame_packet.scene_draw_data = se::graphics::CollectDrawData(entity_subsystem.GetMainWorld().GetWorld());

    // TODO: GPU 리소스 해제 로직
    // - 현재 Bump Pointer 할당이라 개별 VRAM 회수 불가 (UnloadMesh는 매핑만 제거)
    // - Defragmentation 구현 후: ECS에서 참조되지 않는 mesh_id를 주기적으로 스캔하여 UnloadMesh() 호출
    // - 씬 전환 시: GpuResourceManager 전체 리셋 고려

    const se::graphics::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();

    // RenderFrame 람다 내에서 패스 조립
    render_subsystem->RenderFrame(
        // GPU Resource Upload
        [&](SDL_GPUCommandBuffer* cmd)
        {
            EnsureMeshesResident(cmd, frame_packet.scene_draw_data);

            if (DebugDrawSubsystem* debug_subsystem = se::GetSubsystem<DebugDrawSubsystem>())
            {
                debug_subsystem->UploadToGpu(cmd);
            }

            if (gizmo_subsystem)
            {
                for (const StringName& vp_id : viewport_subsystem.GetViewports() | std::views::keys)
                {
                    if (const auto vp_list = gizmo_subsystem->FindDrawList(vp_id))
                    {
                        vp_list->UploadToGpu(cmd);
                    }
                }
            }
        },

        // RenderPass Setup
        [&](se::graphics::RGTextureHandle swapchain_handle, se::graphics::RenderGraphBuilder& builder)
        {
            Array<graphics::RGTextureHandle> viewport_color_handles;
            viewport_color_handles.Reserve(viewport_subsystem.GetViewports().Len());

            for (const auto& [viewport_id, state] : viewport_subsystem.GetViewports())
            {
                if (ui_subsystem.GetPanel(viewport_id)->IsVisible())
                {
                    if (const auto tex_resource = render_subsystem->GetRenderDevice().GetTexture(state.color_texture))
                    {
                        const se::graphics::RenderView& render_view = frame_packet.render_views.Emplace(state.render_view);

                        // ColorTarget Handle 생성
                        const se::graphics::RGTextureHandle color_handle =
                            builder.ImportTexture(state.color_target_name, tex_resource->handle);

                        // DepthTarget Handle 생성
                        const se::graphics::RGTextureHandle depth_handle =
                            builder.CreateTexture(state.depth_target_name, {
                                .type = SDL_GPU_TEXTURETYPE_2D,
                                .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
                                .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                                .width = render_view.width,
                                .height = render_view.height,
                                .layer_count_or_depth = 1,
                                .num_levels = 1,
                                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                            });

                        // 최종 렌더링 Pass에서 참고할 수 있도록 추가
                        viewport_color_handles.Push(color_handle);

                        // Entity ID MRT target 조건 판정
                        const bool need_entity_pick =
                            state.is_hovered
                            && !viewport_subsystem.IsAnyCameraActive()
                            && !(gizmo_subsystem && gizmo_subsystem->IsDragging())
                            && pick_subsystem;

                        // entity_id 텍스처를 PickSubsystem으로부터 Import
                        se::graphics::RGTextureHandle entity_id_handle = {};
                        if (need_entity_pick)
                        {
                            if (SDL_GPUTexture* entity_id_tex = pick_subsystem->GetOrCreateEntityIdTexture(render_view.width, render_view.height))
                            {
                                entity_id_handle = builder.ImportTexture("EntityPickTarget", entity_id_tex);
                            }
                        }

                        // 메인 Scene 렌더링 (entity_id_handle이 유효하면 MRT로 entity ID 동시 출력)
                        {
                            builder.AddPass<se::graphics::ForwardScenePass>(
                                frame_packet.scene_draw_data, gpu_manager,
                                render_view, color_handle, depth_handle, entity_id_handle
                            );
                        }

                        // World Grid 렌더링
                        builder.AddPass<WorldGridPass>(state.view_mode, render_view, color_handle, depth_handle);

                        // Debug Line 렌더링
                        if (DebugDrawSubsystem* debug_subsystem = se::GetSubsystem<DebugDrawSubsystem>())
                        {
                            builder.AddPass<se::graphics::DebugLinePass>(
                                *debug_subsystem, render_view, color_handle, depth_handle
                            );
                        }

                        // 기즈모 렌더링
                        if (gizmo_subsystem)
                        {
                            if (const auto vp_draw_list = gizmo_subsystem->FindDrawList(viewport_id))
                            {
                                builder.AddPass<GizmoPass>(
                                    *vp_draw_list, render_view, color_handle, depth_handle
                                );

                                // 호버된 뷰포트에서만 GPU Color Picking 패스 실행
                                if (
                                    state.is_hovered
                                    && !viewport_subsystem.IsAnyCameraActive()
                                    && !gizmo_subsystem->IsDragging()
                                    && gizmo_subsystem->GetPickTexture()
                                )
                                {
                                    const se::graphics::RGTextureHandle pick_handle =
                                        builder.ImportTexture("GizmoPickTarget", gizmo_subsystem->GetPickTexture());

                                    builder.AddPass<GizmoPickPass>(
                                        *vp_draw_list, render_view, pick_handle, state.cursor_viewport_pos
                                    );
                                }
                            }
                        }
                    }
                }
            }

            // Swapchain 핸들을 생성자 DI로 전달
            builder.AddPass<EditorUIPass>(swapchain_handle, std::move(viewport_color_handles));
        }
    );

    // GPU Readback: pick 텍스처에서 hovered axis 판정
    if (gizmo_subsystem)
    {
        gizmo_subsystem->PerformPick();
    }

    // GPU Readback: entity pick 텍스처에서 커서 아래 Entity ID 판정
    if (pick_subsystem)
    {
        if (const auto hovered_info = viewport_subsystem.GetHoveredViewportInfo())
        {
            pick_subsystem->PerformPick(hovered_info->cursor_viewport_pos);
        }
    }
}

// TODO: 장기적으로 RenderGraph의 Transfer Pass로 통합 예정
void EditorApplication::EnsureMeshesResident(SDL_GPUCommandBuffer* cmd, const graphics::SceneDrawData& in_scene_data)
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
        MeshCookKey cook_key;
    };
    HashMap<asset::AssetId, PendingUpload> pending;

    for (const graphics::DrawCommand& draw_cmd : in_scene_data.opaque_commands)
    {
        // 이미 pending에 있으면 건너뜀
        if (pending.Contains(draw_cmd.mesh_id))
        {
            continue;
        }

        // Fast path: GPU에 이미 올라와 있으면 hash만 in-place 비교 (0 malloc)
        bool is_reimport = false;
        if (gpu_manager.GetSlice(draw_cmd.mesh_id).HasValue())
        {
            bool up_to_date = false;
            registry.ReadRecord(draw_cmd.mesh_id, [&](const asset::AssetRecord& record)
            {
                if (const auto prev = uploaded_mesh_hashes.Find(draw_cmd.mesh_id))
                {
                    up_to_date = prev->source_hash == record.metadata.source_hash
                        && prev->settings_hash == record.metadata.settings_hash;
                }
            });

            if (up_to_date)
            {
                continue;
            }

            // cook_key 불일치 또는 미기록 -> 에셋이 reimport됨 (Pool과 GPU를 무효화)
            gpu_manager.UnloadMesh(draw_cmd.mesh_id);
            asset_subsystem->GetPool().Remove(draw_cmd.mesh_id);
            is_reimport = true;
        }

        // Slow path: 새 메시 또는 reimport -> 경로와 해시를 복사
        asset::AssetPath asset_path;
        MeshCookKey cook_key;
        if (!registry.ReadRecord(draw_cmd.mesh_id, [&](const asset::AssetRecord& record)
        {
            asset_path = record.logical_path;
            cook_key = {
                .source_hash = record.metadata.source_hash,
                .settings_hash = record.metadata.settings_hash,
            };
        }))
        {
            continue;
        }

        if (is_reimport)
        {
            ConsoleLog(ELogLevel::Info, "GPU mesh invalidated (reimport detected): {}", asset_path.ToString());
        }

        pending.Insert(draw_cmd.mesh_id, {
            .path = std::move(asset_path),
            .cook_key = std::move(cook_key),
        });
    }

    if (pending.IsEmpty())
    {
        return;
    }

    // 각 메시를 Registry에서 경로 조회 -> Load -> GPU 업로드
    for (const auto& [mesh_id, info] : pending)
    {
        // Load를 호출하여 DDC에서 역직렬화 -> Pool에 등록
        const asset::AssetHandle<asset::StaticMesh> handle = asset_subsystem->Load<asset::StaticMesh>(info.path);
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
            cmd,
            mesh_id,
            mesh->vertices.Data(),
            static_cast<uint32>(mesh->vertices.Len() * sizeof(graphics::Vertex)),
            mesh->indices.Data(),
            static_cast<uint32>(mesh->indices.Len() * sizeof(uint32))
        );

        if (success)
        {
            uploaded_mesh_hashes.Insert(mesh_id, {
                .source_hash = info.cook_key.source_hash,
                .settings_hash = info.cook_key.settings_hash,
            });
        }
    }
}
} // namespace se::editor
