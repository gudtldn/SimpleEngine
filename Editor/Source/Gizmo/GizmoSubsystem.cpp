#include "SimpleEditor/Gizmo/GizmoSubsystem.h"

#include "SimpleEditor/Camera/EditorCameraState.h"
#include "SimpleEditor/Core/SelectionSubsystem.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Input/MouseButton.h"
#include "SimpleEngine/Core/Math/TransformUtility.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include <utility>

#include "SimpleEngine/ECS/Components/ParentComponent.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(GizmoSubsystem)
    .DependsOn<RenderSubsystem, SelectionSubsystem>()
    .UpdateDependsOn<EditorViewportSubsystem>();

SE_BEGIN_REFLECT(GizmoSubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(GizmoSubsystem)

bool GizmoSubsystem::Initialize()
{
    render_device = &GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();

    // 1x1 R32_UINT pick 텍스쳐
    constexpr SDL_GPUTextureCreateInfo tex_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R32_UINT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        .width = 1,
        .height = 1,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    pick_texture_rid = render_device->CreateTexture(tex_info, "Gizmo_Pick");
    if (!render_device->IsValidTexture(pick_texture_rid))
    {
        return false;
    }

    // 4바이트 download transfer buffer (GPU -> CPU readback)
    constexpr SDL_GPUTransferBufferCreateInfo tb_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = sizeof(uint32),
    };
    download_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &tb_info);
    return download_buffer != nullptr;
}

void GizmoSubsystem::Release()
{
    if (download_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), std::exchange(download_buffer, nullptr));
    }
    if (render_device->IsValidTexture(pick_texture_rid))
    {
        render_device->DestroyTexture(std::exchange(pick_texture_rid, {}));
    }
    draw_lists.Clear();
}

void GizmoSubsystem::DrawGizmos()
{
    // 모든 draw list 초기화
    for (const auto& list : draw_lists | std::views::values)
    {
        list->Clear();
    }

    const auto [selection_subsystem, entity_subsystem, viewport_subsystem] =
        se::GetSubsystems<const SelectionSubsystem, const EntitySubsystem, const EditorViewportSubsystem>();

    if (!selection_subsystem || !entity_subsystem || !viewport_subsystem)
    {
        return;
    }

    const auto selected_entity_opt = selection_subsystem->GetSelection().GetPrimarySelectedEntity();
    if (!selected_entity_opt)
    {
        return;
    }

    const World& world = entity_subsystem->GetMainWorld().GetWorld();
    const auto global_tf = world.TryGetComponent<GlobalTransformComponent>(*selected_entity_opt);
    if (!global_tf)
    {
        return;
    }

    const Vector3 center = math::TransformUtility::DecomposeTranslation(global_tf->value);

    // 각 뷰포트별로 해당 뷰포트의 기즈모 모드/좌표계로 기즈모를 조립
    for (const auto& [viewport_id, state] : viewport_subsystem->GetViewports())
    {
        GizmoDrawList& list = GetOrCreateDrawList(viewport_id);
        list.SetCenter(center);

        const EditorCameraState& camera = state.GetActiveCamera();
        const Vector3 direction_to_widget = (center - camera.position).GetNormalized();
        list.SetDirectionToWidget(direction_to_widget);

        const Quaternion rotation = (state.coordinate_space == ECoordinateSpace::Local)
            ? math::TransformUtility::DecomposeRotation(global_tf->value)
            : Quaternion::Identity();

        renderer.SetMode(state.gizmo_mode);
        renderer.Draw(list, rotation);
    }
}

void GizmoSubsystem::PerformPick()
{
    if (!pick_texture_rid || !download_buffer)
    {
        return;
    }

    // 호버된 뷰포트의 draw list가 비어있으면 pick 불필요
    const bool has_geometry = [this] -> bool
    {
        for (const auto& list : draw_lists | std::views::values)
        {
            if (list->GetLineVertexCount() > 0 || list->GetTriangleVertexCount() > 0)
            {
                return true;
            }
        }
        return false;
    }();

    if (!has_geometry)
    {
        hovered_axis = EGizmoAxis::None;
        renderer.SetHighlightAxis(EGizmoAxis::None);
        return;
    }

    // GPU readback: 1x1 텍스쳐 -> transfer buffer -> CPU
    SDL_GPUDevice* raw_device = render_device->GetRawDevice();
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(raw_device);
    if (!cmd)
    {
        hovered_axis = EGizmoAxis::None;
        renderer.SetHighlightAxis(EGizmoAxis::None);
        return;
    }

    SDL_GPUTexture* texture = GetPickTexture();
    if (!texture)
    {
        SDL_CancelGPUCommandBuffer(cmd);
        hovered_axis = EGizmoAxis::None;
        renderer.SetHighlightAxis(EGizmoAxis::None);
        return;
    }

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    {
        const SDL_GPUTextureRegion src = {
            .texture = texture,
            .w = 1, .h = 1, .d = 1,
        };
        const SDL_GPUTextureTransferInfo dst = {
            .transfer_buffer = download_buffer,
            .offset = 0,
        };
        SDL_DownloadFromGPUTexture(copy, &src, &dst);
    }
    SDL_EndGPUCopyPass(copy);

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(raw_device, true, &fence, 1);
    SDL_ReleaseGPUFence(raw_device, fence);

    uint32 pick_id = 0;
    if (const void* data = SDL_MapGPUTransferBuffer(raw_device, download_buffer, false))
    {
        pick_id = *static_cast<const uint32*>(data);
        SDL_UnmapGPUTransferBuffer(raw_device, download_buffer);
    }

    hovered_axis = GizmoRenderer::DecodePickID(pick_id);
    renderer.SetHighlightAxis(hovered_axis);
}

SDL_GPUTexture* GizmoSubsystem::GetPickTexture() const
{
    if (const auto resource = render_device->GetTexture(pick_texture_rid))
    {
        return resource->handle;
    }
    return nullptr;
}

void GizmoSubsystem::Update(double /*delta_time*/)
{
    HandleInteraction();
}

void GizmoSubsystem::HandleInteraction()
{
    const auto [selection_subsystem, entity_subsystem, viewport_subsystem, input_subsystem] =
        se::GetSubsystems<SelectionSubsystem, EntitySubsystem, EditorViewportSubsystem, const InputSubsystem>();

    if (!selection_subsystem || !entity_subsystem || !viewport_subsystem || !input_subsystem)
    {
        return;
    }

    // 카메라 조작 중에는 기즈모 드래그 차단
    if (viewport_subsystem->IsAnyCameraActive())
    {
        if (interaction.IsDragging())
        {
            interaction.EndDrag();
        }
        return;
    }

    const auto selected_entity = selection_subsystem->GetSelection().GetPrimarySelectedEntity();
    if (!selected_entity)
    {
        if (interaction.IsDragging())
        {
            interaction.EndDrag();
        }
        return;
    }

    const auto vp_info = viewport_subsystem->GetFocusedViewportInfo();
    if (!vp_info)
    {
        return;
    }

    // 드래그 시작 (LMB 눌림 + hover 중인 축이 있음)
    // hovered_axis는 호버된 뷰포트의 pick 결과이므로, 호버 뷰포트 기준으로 드래그 시작
    // (클릭 시 ImGui가 해당 뷰포트를 자동 focus하므로, 이후 업데이트는 focused 뷰포트 사용)
    if (!interaction.IsDragging() && input_subsystem->IsMouseButtonPressed(EMouseButton::Left) && hovered_axis != EGizmoAxis::None)
    {
        const auto hovered_vp = viewport_subsystem->GetHoveredViewportInfo();
        if (!hovered_vp)
        {
            return;
        }

        World& world = entity_subsystem->GetMainWorld().GetWorld();
        const auto global_tf = world.TryGetComponent<GlobalTransformComponent>(*selected_entity);
        if (!global_tf)
        {
            return;
        }

        const Vector3 center = math::TransformUtility::DecomposeTranslation(global_tf->value);
        const Quaternion rotation = (hovered_vp->coordinate_space == ECoordinateSpace::Local)
            ? math::TransformUtility::DecomposeRotation(global_tf->value)
            : Quaternion::Identity();

        interaction.BeginDrag(
            hovered_vp->gizmo_mode,
            hovered_axis,
            hovered_vp->coordinate_space == ECoordinateSpace::Local,
            hovered_vp->cursor_viewport_pos,
            center,
            rotation,
            hovered_vp->render_view
        );

        // Alt+드래그: 엔티티 복제
        if (input_subsystem->HasModifier(EModifier::Alt))
        {
            // TODO: 선택된 엔티티를 복제하고, 복제본을 드래그 대상으로 전환
        }

        return;
    }

    // 드래그 종료 (LMB 뗌)
    if (interaction.IsDragging() && input_subsystem->IsMouseButtonReleased(EMouseButton::Left))
    {
        interaction.EndDrag();
        return;
    }

    // 드래그 업데이트 (LMB 유지 중)
    if (interaction.IsDragging() && input_subsystem->IsMouseButtonDown(EMouseButton::Left))
    {
        const auto result = interaction.UpdateDrag(vp_info->cursor_viewport_pos, vp_info->render_view);

        World& world = entity_subsystem->GetMainWorld().GetWorld();
        const auto transform = world.TryGetComponent<TransformComponent>(*selected_entity);
        if (!transform)
        {
            return;
        }

        switch (vp_info->gizmo_mode)
        {
        case EGizmoMode::Translate:
        {
            // 부모의 변환(스케일/회전)이 자식에게 중복 적용되지 않도록, 월드 이동량을 부모의 로컬 공간으로 역변환
            const Vector3 local_delta = [&]
            {
                if (const auto parent_comp = world.TryGetComponent<ParentComponent>(*selected_entity))
                {
                    if (const auto parent_global = world.TryGetComponent<GlobalTransformComponent>(parent_comp->parent))
                    {
                        const Matrix4x4 parent_inverse = parent_global->value.Inverse();
                        return parent_inverse.TransformVector(result.translation_delta);
                    }
                }
                return result.translation_delta;
            }();

            transform->position = transform->position + local_delta;
            transform->dirty = true;

            // Shift+드래그: 카메라가 이동량만큼 따라감
            if (input_subsystem->HasModifier(EModifier::Shift))
            {
                const StringName focused_id = viewport_subsystem->GetFocusedViewportId();
                if (const auto camera = viewport_subsystem->GetViewportCamera(focused_id))
                {
                    // 카메라 이동에 의한 ray 원점 변화를 드래그 기준점에 반영
                    camera->position += result.translation_delta;
                    interaction.OffsetDragReference(result.translation_delta);
                }
            }
            break;
        }
        case EGizmoMode::Rotate:
        {
            if (result.is_local_rotation)
            {
                // Local: Q * FromAxisAngle(basis_axis, θ) -> right-multiply = 로컬 프레임 해석
                transform->rotation = transform->rotation * result.rotation_delta;
            }
            else
            {
                // World: FromAxisAngle(basis_axis, θ) * Q -> left-multiply = 월드 프레임 해석
                transform->rotation = result.rotation_delta * transform->rotation;
            }
            transform->dirty = true;
            break;
        }
        case EGizmoMode::Scale:
        {
            transform->scale = transform->scale + result.scale_delta;
            transform->dirty = true;
            break;
        }
        }
    }
}

Optional<GizmoDrawList&> GizmoSubsystem::FindDrawList(const StringName& viewport_id)
{
    return draw_lists.Find(viewport_id).Map([](auto& list) -> GizmoDrawList&
    {
        return *list;
    });
}

Optional<const GizmoDrawList&> GizmoSubsystem::FindDrawList(const StringName& viewport_id) const
{
    return draw_lists.Find(viewport_id).Map([](const auto& list) -> const GizmoDrawList&
    {
        return *list;
    });
}

GizmoDrawList& GizmoSubsystem::GetOrCreateDrawList(const StringName& viewport_id)
{
    return *draw_lists.Entry(viewport_id).OrInsertWith([this]
    {
        return std::make_unique<GizmoDrawList>(*render_device);
    });
}
} // namespace se::editor
