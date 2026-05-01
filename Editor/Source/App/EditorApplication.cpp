#include "SimpleEditor/App/EditorApplication.h"
#include "SimpleEditor/App/EditorFrameInput.h"

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
#include "SimpleEngine/Asset/BuiltinAssets.h"
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

    const auto [entity_subsystem, ui_subsystem, viewport_subsystem, asset_subsystem] =
        se::GetSubsystemsChecked<const EntitySubsystem, const EditorUISubsystem, const EditorViewportSubsystem, AssetSubsystem>();

    // FramePacket 조립 (SceneDrawData 수집)
    se::FramePacket frame_packet;
    static uint64 frame_counter = 0;
    frame_packet.frame_number = ++frame_counter; // TODO: 나중에 frame_counter를 통합 관리하는 구조체 만들기
    frame_packet.scene_draw_data = se::CollectDrawData(entity_subsystem.GetMainWorld().GetWorld(), asset_subsystem);

    // GPU 업로드 요청 수집 (게임 스레드에서 Asset Load + residency 체크)
    PrepareGpuUploads(frame_packet);
    // TODO: GPU 리소스 해제 로직
    // - 현재 Bump Pointer 할당이라 개별 VRAM 회수 불가 (UnloadMesh는 매핑만 제거)
    // - Defragmentation 구현 후: ECS에서 참조되지 않는 mesh_id를 주기적으로 스캔하여 UnloadMesh() 호출
    // - 씬 전환 시: GpuResourceManager 전체 리셋 고려

    const se::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();

    // 게임 스레드에서 뷰포트별 렌더 데이터 스냅샷 수집
    Array<ViewportRenderInput> viewport_inputs;
    viewport_inputs.Reserve(viewport_subsystem.GetViewports().Len());

    // PickSubsystem은 entity_id 텍스처를 하나만 소유하므로 프레임당 한 뷰포트에서만 피킹하도록
    bool entity_pick_assigned = false;
    for (const auto& [viewport_id, state] : viewport_subsystem.GetViewports())
    {
        if (!ui_subsystem.GetPanel(viewport_id)->IsVisible())
        {
            continue;
        }

        const auto tex_resource = render_subsystem->GetRenderDevice().GetTexture(state.color_texture);
        if (!tex_resource)
        {
            continue;
        }

        const bool need_entity_pick =
            !entity_pick_assigned
            && state.is_hovered
            && !viewport_subsystem.IsAnyCameraActive()
            && !(gizmo_subsystem && gizmo_subsystem->IsDragging())
            && pick_subsystem;

        SDL_GPUTexture* entity_id_tex = nullptr;
        if (need_entity_pick)
        {
            entity_id_tex = pick_subsystem->GetOrCreateEntityIdTexture(state.render_view.width, state.render_view.height);
            entity_pick_assigned = true;
        }

        GizmoDrawList* gizmo_draw_list = nullptr;
        if (gizmo_subsystem)
        {
            if (const auto draw_list_opt = gizmo_subsystem->FindDrawList(viewport_id))
            {
                gizmo_draw_list = &(*draw_list_opt);
            }
        }

        const bool show_gizmo_pick_pass =
            gizmo_subsystem != nullptr
            && state.is_hovered
            && !viewport_subsystem.IsAnyCameraActive()
            && !gizmo_subsystem->IsDragging()
            && gizmo_subsystem->GetPickTexture() != nullptr;

        viewport_inputs.Push({
            .render_view = state.render_view,
            .color_target_name = state.color_target_name,
            .depth_target_name = state.depth_target_name,
            .view_mode = state.view_mode,
            .need_entity_pick = need_entity_pick && entity_id_tex != nullptr,
            .cursor_viewport_pos = state.cursor_viewport_pos,
            .color_texture_raw = tex_resource->handle,
            .entity_id_texture = entity_id_tex,
            .gizmo_pick_texture = show_gizmo_pick_pass ? gizmo_subsystem->GetPickTexture() : nullptr,
            .gizmo_draw_list = gizmo_draw_list,
            .show_gizmo_pick_pass = show_gizmo_pick_pass,
        });

        frame_packet.render_views.Push(state.render_view);
    }

    // RenderFrame 람다 내에서 패스 조립
    render_subsystem->RenderFrame(
        // GPU Resource Upload
        [&](SDL_GPUCommandBuffer* cmd)
        {
            ExecuteGpuUploads(cmd, frame_packet);

            if (DebugDrawSubsystem* debug_subsystem = se::GetSubsystem<DebugDrawSubsystem>())
            {
                debug_subsystem->UploadToGpu(cmd);
            }

            for (const ViewportRenderInput& input : viewport_inputs)
            {
                if (input.gizmo_draw_list)
                {
                    input.gizmo_draw_list->UploadToGpu(cmd);
                }
            }
        },

        // RenderPass Setup
        [&](se::RGTextureHandle swapchain_handle, se::RenderGraphBuilder& builder)
        {
            Array<RGTextureHandle> viewport_color_handles;
            viewport_color_handles.Reserve(viewport_inputs.Len());

            for (const ViewportRenderInput& input : viewport_inputs)
            {
                const se::RenderView& render_view = input.render_view;

                // ColorTarget Handle 생성
                const se::RGTextureHandle color_handle =
                    builder.ImportTexture(input.color_target_name, input.color_texture_raw);

                // DepthTarget Handle 생성
                const se::RGTextureHandle depth_handle =
                    builder.CreateTexture(input.depth_target_name, {
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

                // entity_id 텍스처를 PickSubsystem으로부터 Import
                se::RGTextureHandle entity_id_handle = {};
                if (input.need_entity_pick && input.entity_id_texture)
                {
                    entity_id_handle = builder.ImportTexture("EntityPickTarget", input.entity_id_texture);
                }

                // 메인 Scene 렌더링 (entity_id_handle이 유효하면 MRT로 entity ID 동시 출력)
                builder.AddPass<se::ForwardScenePass>(
                    frame_packet.scene_draw_data, gpu_manager,
                    render_view, color_handle, depth_handle, entity_id_handle
                );

                // World Grid 렌더링
                builder.AddPass<WorldGridPass>(input.view_mode, render_view, color_handle, depth_handle);

                // Debug Line 렌더링
                if (DebugDrawSubsystem* debug_subsystem = se::GetSubsystem<DebugDrawSubsystem>())
                {
                    builder.AddPass<se::DebugLinePass>(
                        *debug_subsystem, render_view, color_handle, depth_handle
                    );
                }

                // 기즈모 렌더링
                if (input.gizmo_draw_list)
                {
                    builder.AddPass<GizmoPass>(
                        *input.gizmo_draw_list, render_view, color_handle, depth_handle
                    );

                    // 호버된 뷰포트에서만 GPU Color Picking 패스 실행
                    if (input.show_gizmo_pick_pass && input.gizmo_pick_texture)
                    {
                        const se::RGTextureHandle pick_handle =
                            builder.ImportTexture("GizmoPickTarget", input.gizmo_pick_texture);

                        builder.AddPass<GizmoPickPass>(
                            *input.gizmo_draw_list, render_view, pick_handle, input.cursor_viewport_pos
                        );
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
void EditorApplication::PrepareGpuUploads(FramePacket& fp)
{
    const RenderSubsystem* render_subsystem = se::GetSubsystem<RenderSubsystem>();
    auto* asset_subsystem = se::GetSubsystem<AssetSubsystem>();
    if (!render_subsystem || !asset_subsystem)
    {
        return;
    }

    GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();
    const AssetRegistry& registry = asset_subsystem->GetRegistry();

    // --- Mesh residency 체크 ---
    // [핵심] opaque_commands 전체 루프를 돌지 않고, 이미 중복이 제거된 pinned_meshes 맵만 순회한다!
    for (const auto& [mesh_id, handle] : fp.scene_draw_data.pinned_meshes)
    {
        bool is_reimport = false;

        if (gpu_manager.GetSlice(mesh_id).HasValue())
        {
            bool up_to_date = false;
            registry.ReadRecord(mesh_id, [&](const AssetRecord& record)
            {
                if (const auto prev = gpu_manager.GetMeshResidencyKey(mesh_id))
                {
                    up_to_date = prev->source_hash == record.metadata.source_hash
                        && prev->settings_hash == record.metadata.settings_hash;
                }
            });

            if (up_to_date) continue; // GPU에 이미 최신 버전이 올라가 있음

            // Reimport 감지
            gpu_manager.UnloadMesh(mesh_id);
            asset_subsystem->GetPool().Remove(mesh_id);
            is_reimport = true;
        }

        // 로드 및 업로드 큐 등록
        AssetPath asset_path;
        GpuResourceManager::MeshResidencyKey cook_key;
        if (!registry.ReadRecord(mesh_id, [&](const AssetRecord& record) {
            asset_path = record.logical_path;
            cook_key = { record.metadata.source_hash, record.metadata.settings_hash };
        })) continue;

        if (is_reimport) ConsoleLog(ELogLevel::Info, "GPU mesh invalidated: {}", asset_path.ToString());

        // 다시 로드해서 핸들 소유권 강제 (DDC 역직렬화)
        AssetHandle<StaticMesh> fresh_handle = asset_subsystem->Load<StaticMesh>(asset_path);
        if (fresh_handle && !fresh_handle->vertices.IsEmpty())
        {
            fp.mesh_upload_requests.Push({
                .mesh_id = mesh_id,
                .handle = std::move(fresh_handle),
                .new_residency_key = cook_key,
            });
        }
    }

    // --- Texture residency 체크 ---
    // White1x1 빌트인 보장
    if (!gpu_manager.GetTexture(BuiltinAssetIds::White1x1).HasValue())
    {
        if (AssetHandle<Texture2D> handle = asset_subsystem->Find<Texture2D>(BuiltinAssetIds::White1x1))
        {
            fp.texture_upload_requests.Push({
                .texture_id = BuiltinAssetIds::White1x1,
                .handle = std::move(handle),
            });
        }
    }

    // [핵심] 씬에 사용된 고유 텍스처들만 싹 훑는다.
    HashSet<AssetId> queued_textures;
    queued_textures.Insert(BuiltinAssetIds::White1x1);

    for (const TextureBinding& binding : fp.scene_draw_data.material_cache.binding_arena)
    {
        if (queued_textures.Contains(binding.texture_id))
        {
            continue;
        }
        if (!gpu_manager.GetTexture(binding.texture_id).HasValue())
        {
            if (AssetHandle<Texture2D> handle = asset_subsystem->Find<Texture2D>(binding.texture_id))
            {
                queued_textures.Insert(binding.texture_id);
                fp.texture_upload_requests.Push({
                    .texture_id = binding.texture_id,
                    .handle = std::move(handle),
                });
            }
        }
    }
}

void EditorApplication::ExecuteGpuUploads(SDL_GPUCommandBuffer* cmd, const FramePacket& fp)
{
    const RenderSubsystem* render_subsystem = se::GetSubsystem<const RenderSubsystem>();
    if (!render_subsystem)
    {
        return;
    }

    GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();

    for (const MeshUploadRequest& request : fp.mesh_upload_requests)
    {
        const StaticMesh* mesh = request.handle.Get();
        if (!mesh)
        {
            continue;
        }

        const bool success = gpu_manager.UploadMesh(
            cmd,
            request.mesh_id,
            mesh->vertices.Data(),
            static_cast<uint32>(mesh->vertices.Len() * sizeof(StaticVertex)),
            mesh->indices.Data(),
            static_cast<uint32>(mesh->indices.Len() * sizeof(uint32))
        );

        if (success)
        {
            gpu_manager.SetMeshResidencyKey(request.mesh_id, request.new_residency_key);
        }
    }

    for (const TextureUploadRequest& request : fp.texture_upload_requests)
    {
        if (const Texture2D* tex = request.handle.Get())
        {
            gpu_manager.UploadTexture(cmd, request.texture_id, *tex);
        }
    }
}
} // namespace se::editor
