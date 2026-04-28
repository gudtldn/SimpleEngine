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
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
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
using namespace asset;

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
    static uint64 frame_counter = 0;
    frame_packet.frame_number = ++frame_counter; // TODO: 나중에 frame_counter를 통합 관리하는 구조체 만들기
    frame_packet.scene_draw_data = se::graphics::CollectDrawData(entity_subsystem.GetMainWorld().GetWorld());

    // 머티리얼 데이터 값 복사
    PrepareMaterialData(frame_packet);

    // GPU 업로드 요청 수집 (게임 스레드에서 Asset Load + residency 체크)
    PrepareGpuUploads(frame_packet);

    // TODO: GPU 리소스 해제 로직
    // - 현재 Bump Pointer 할당이라 개별 VRAM 회수 불가 (UnloadMesh는 매핑만 제거)
    // - Defragmentation 구현 후: ECS에서 참조되지 않는 mesh_id를 주기적으로 스캔하여 UnloadMesh() 호출
    // - 씬 전환 시: GpuResourceManager 전체 리셋 고려

    const se::graphics::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();
    const se::graphics::SamplerCache& sampler_cache = render_subsystem->GetSamplerCache();

    // 게임 스레드에서 뷰포트별 렌더 데이터 스냅샷 수집
    Array<ViewportRenderInput> viewport_inputs;
    viewport_inputs.Reserve(viewport_subsystem.GetViewports().Len());
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
            state.is_hovered
            && !viewport_subsystem.IsAnyCameraActive()
            && !(gizmo_subsystem && gizmo_subsystem->IsDragging())
            && pick_subsystem;

        SDL_GPUTexture* entity_id_tex = nullptr;
        if (need_entity_pick)
        {
            entity_id_tex = pick_subsystem->GetOrCreateEntityIdTexture(state.render_view.width, state.render_view.height);
        }

        GizmoDrawList* gizmo_draw_list = nullptr;
        if (gizmo_subsystem)
        {
            if (auto draw_list_opt = gizmo_subsystem->FindDrawList(viewport_id))
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
        [&](se::graphics::RGTextureHandle swapchain_handle, se::graphics::RenderGraphBuilder& builder)
        {
            Array<graphics::RGTextureHandle> viewport_color_handles;
            viewport_color_handles.Reserve(viewport_inputs.Len());

            for (const ViewportRenderInput& input : viewport_inputs)
            {
                const se::graphics::RenderView& render_view = input.render_view;

                // ColorTarget Handle 생성
                const se::graphics::RGTextureHandle color_handle =
                    builder.ImportTexture(input.color_target_name, input.color_texture_raw);

                // DepthTarget Handle 생성
                const se::graphics::RGTextureHandle depth_handle =
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
                se::graphics::RGTextureHandle entity_id_handle = {};
                if (input.need_entity_pick && input.entity_id_texture)
                {
                    entity_id_handle = builder.ImportTexture("EntityPickTarget", input.entity_id_texture);
                }

                // 메인 Scene 렌더링 (entity_id_handle이 유효하면 MRT로 entity ID 동시 출력)
                builder.AddPass<se::graphics::ForwardScenePass>(
                    frame_packet.scene_draw_data, gpu_manager, sampler_cache,
                    render_view, color_handle, depth_handle, entity_id_handle
                );

                // World Grid 렌더링
                builder.AddPass<WorldGridPass>(input.view_mode, render_view, color_handle, depth_handle);

                // Debug Line 렌더링
                if (DebugDrawSubsystem* debug_subsystem = se::GetSubsystem<DebugDrawSubsystem>())
                {
                    builder.AddPass<se::graphics::DebugLinePass>(
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
                        const se::graphics::RGTextureHandle pick_handle =
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
void EditorApplication::PrepareGpuUploads(graphics::FramePacket& fp)
{
    const auto [render_subsystem, asset_subsystem] = se::GetSubsystems<const RenderSubsystem, AssetSubsystem>();
    if (!render_subsystem || !asset_subsystem)
    {
        return;
    }

    graphics::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();
    const AssetRegistry& registry = asset_subsystem->GetRegistry();

    // --- Mesh residency 체크 ---
    // 업로드가 필요한 메시를 수집 (아직 업로드되지 않은 데이터 또는 cook_key 변경)
    struct PendingMesh
    {
        AssetPath path;
        graphics::GpuResourceManager::MeshResidencyKey cook_key;
    };
    HashMap<AssetId, PendingMesh> pending_meshes;

    for (const graphics::DrawCommand& draw_cmd : fp.scene_draw_data.opaque_commands)
    {
        // 이미 pending에 있으면 건너뜀
        if (pending_meshes.Contains(draw_cmd.mesh_id))
        {
            continue;
        }

        // Fast path: GPU에 이미 올라와 있으면 hash만 in-place 비교 (0 malloc)
        bool is_reimport = false;
        if (gpu_manager.GetSlice(draw_cmd.mesh_id).HasValue())
        {
            bool up_to_date = false;
            registry.ReadRecord(draw_cmd.mesh_id, [&](const AssetRecord& record)
            {
                if (const auto prev = gpu_manager.GetMeshResidencyKey(draw_cmd.mesh_id))
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
        AssetPath asset_path;
        graphics::GpuResourceManager::MeshResidencyKey cook_key;
        if (!registry.ReadRecord(draw_cmd.mesh_id, [&](const AssetRecord& record)
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

        pending_meshes.Insert(draw_cmd.mesh_id, {
            .path = std::move(asset_path),
            .cook_key = std::move(cook_key),
        });
    }

    // 각 메시를 Registry에서 경로 조회 -> Load -> GPU 업로드
    for (const auto& [mesh_id, info] : pending_meshes)
    {
        // Load를 호출하여 DDC에서 역직렬화 -> Pool에 등록
        AssetHandle<StaticMesh> handle = asset_subsystem->Load<StaticMesh>(info.path);
        if (!handle)
        {
            ConsoleLog(ELogLevel::Warning, "PrepareGpuUploads: Mesh load failed for {}", info.path.ToString());
            continue;
        }

        if (handle->vertices.IsEmpty())
        {
            ConsoleLog(ELogLevel::Warning, "PrepareGpuUploads: Empty mesh data for {}", info.path.ToString());
            continue;
        }

        fp.mesh_upload_requests.Push({
            .mesh_id = mesh_id,
            .handle = std::move(handle),
            .new_residency_key = info.cook_key,
        });
    }

    // --- Texture residency 체크 ---
    // White1x1 빌트인
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

    // 씬 텍스처 (FrameMaterialCache binding_arena 순회)
    HashSet<AssetId> queued_textures;
    queued_textures.Insert(BuiltinAssetIds::White1x1);
    for (const graphics::TextureBinding& binding : fp.scene_draw_data.material_cache.binding_arena)
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

void EditorApplication::ExecuteGpuUploads(SDL_GPUCommandBuffer* cmd, const graphics::FramePacket& fp)
{
    const RenderSubsystem* render_subsystem = se::GetSubsystem<const RenderSubsystem>();
    if (!render_subsystem)
    {
        return;
    }

    graphics::GpuResourceManager& gpu_manager = render_subsystem->GetResourceManager();

    for (const graphics::MeshUploadRequest& request : fp.mesh_upload_requests)
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
            static_cast<uint32>(mesh->vertices.Len() * sizeof(graphics::StaticVertex)),
            mesh->indices.Data(),
            static_cast<uint32>(mesh->indices.Len() * sizeof(uint32))
        );

        if (success)
        {
            gpu_manager.SetMeshResidencyKey(request.mesh_id, request.new_residency_key);
        }
    }

    for (const graphics::TextureUploadRequest& request : fp.texture_upload_requests)
    {
        if (const Texture2D* tex = request.handle.Get())
        {
            gpu_manager.UploadTexture(cmd, request.texture_id, *tex);
        }
    }
}

void EditorApplication::PrepareMaterialData(graphics::FramePacket& fp)
{
    const AssetSubsystem* asset_subsystem = se::GetSubsystem<const AssetSubsystem>();
    if (!asset_subsystem)
    {
        return;
    }

    graphics::FrameMaterialCache& cache = fp.scene_draw_data.material_cache;
    cache.Clear();

    for (graphics::DrawCommand& draw_cmd : fp.scene_draw_data.opaque_commands)
    {
        // 동일 material_id는 캐시 hit
        if (const auto slot_idx = cache.material_to_slot.Find(draw_cmd.material_id))
        {
            draw_cmd.material_slot_index = *slot_idx;
            continue;
        }

        AssetHandle<MaterialInstance> inst_handle = asset_subsystem->Find<MaterialInstance>(draw_cmd.material_id);
        if (!inst_handle)
        {
            inst_handle = asset_subsystem->Find<MaterialInstance>(BuiltinAssetIds::DefaultLitInstance);
        }

        if (!inst_handle)
        {
            draw_cmd.material_slot_index = std::numeric_limits<uint16>::max();
            continue;
        }

        AssetHandle<Material> mat_handle = asset_subsystem->Find<Material>(inst_handle->parent_material_id);
        if (!mat_handle)
        {
            draw_cmd.material_slot_index = std::numeric_limits<uint16>::max();
            continue;
        }

        graphics::FrameMaterialCache::MaterialSlot slot;

        // UBO pack
        slot.ubo_offset = static_cast<uint32>(cache.ubo_arena.Len());
        SE_ASSERT(inst_handle->parameter_values.Len() <= std::numeric_limits<uint16>::max());
        slot.ubo_size = static_cast<uint16>(inst_handle->parameter_values.Len());
        cache.ubo_arena.PushRange(inst_handle->parameter_values);

        // Texture bindings pack
        slot.binding_offset = static_cast<uint32>(cache.binding_arena.Len());
        for (const graphics::MaterialTextureSlot& tex_slot : mat_handle->texture_slots)
        {
            cache.binding_arena.Push({
                .fragment_slot = tex_slot.fragment_slot,
                .texture_id = inst_handle->GetTextureOrDefault(tex_slot.name, *mat_handle),
                .sampler = tex_slot.sampler,
            });
        }
        SE_ASSERT((cache.binding_arena.Len() - slot.binding_offset) <= std::numeric_limits<uint16>::max());
        slot.binding_count = static_cast<uint16>(cache.binding_arena.Len() - slot.binding_offset);

        SE_ASSERT(cache.slots.Len() <= std::numeric_limits<uint16>::max());
        const uint16 slot_index = static_cast<uint16>(cache.slots.Len());
        cache.slots.Push(slot);
        cache.material_to_slot.Insert(draw_cmd.material_id, slot_index);
        draw_cmd.material_slot_index = slot_index;

        // FramePacket 수명 동안 MaterialInstance가 Evict되지 않도록 Handle을 소유
        fp.pinned_material_handles.Push(std::move(inst_handle));
    }
}
} // namespace se::editor
