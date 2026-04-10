#include "SimpleEditor/Gizmo/GizmoSubsystem.h"

#include "SimpleEditor/Camera/EditorCameraState.h"
#include "SimpleEditor/Core/EditorSubsystem.h"
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


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(GizmoSubsystem)
    .DependsOn<RenderSubsystem>()
    .UpdateDependsOn<EditorViewportSubsystem>();

SE_BEGIN_REFLECT(GizmoSubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(GizmoSubsystem)

bool GizmoSubsystem::Initialize()
{
    render_device = &GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();
    draw_list = std::make_unique<GizmoDrawList>(*render_device);

    // 1x1 R32_UINT pick 텍스처
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
    pick_texture_rid = render_device->CreateTexture(tex_info);
    SE_ASSERT_RELEASE(render_device->IsValidTexture(pick_texture_rid));

    // 4바이트 download transfer buffer
    constexpr SDL_GPUTransferBufferCreateInfo tb_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
        .size = sizeof(uint32),
    };
    pick_download_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &tb_info);
    SE_ASSERT_RELEASE(pick_download_buffer);

    return true;
}

void GizmoSubsystem::Release()
{
    if (pick_download_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), std::exchange(pick_download_buffer, nullptr));
    }
    if (render_device->IsValidTexture(pick_texture_rid))
    {
        render_device->DestroyTexture(std::exchange(pick_texture_rid, {}));
    }
    draw_list.reset();
}

void GizmoSubsystem::DrawGizmos()
{
    draw_list->Clear();

    const auto [editor_subsystem, entity_subsystem, viewport_subsystem] =
        se::GetSubsystems<const EditorSubsystem, const EntitySubsystem, const EditorViewportSubsystem>();

    if (!editor_subsystem || !entity_subsystem || !viewport_subsystem)
    {
        return;
    }

    const auto selected_entity_opt = editor_subsystem->GetSelection().GetPrimarySelectedEntity();
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

    const auto vp_info = viewport_subsystem->GetFocusedViewportInfo();
    if (!vp_info)
    {
        return;
    }

    draw_list->SetCenter(math::TransformUtility::DecomposeTranslation(global_tf->value));

    const EditorCameraState& camera = vp_info->GetActiveCamera();
    const Vector3 direction_to_widget = (draw_list->GetCenter() - camera.position).GetNormalized();
    draw_list->SetDirectionToWidget(direction_to_widget);

    const Quaternion rotation = (vp_info->coordinate_space == ECoordinateSpace::Local)
        ? math::TransformUtility::DecomposeRotation(global_tf->value)
        : Quaternion::Identity();

    renderer.SetMode(vp_info->gizmo_mode);
    renderer.Draw(*draw_list, rotation);
}

void GizmoSubsystem::PerformPick()
{

    if (!pick_texture_rid || !pick_download_buffer)
    {
        return;
    }

    if (draw_list->GetLineVertexCount() == 0 && draw_list->GetTriangleVertexCount() == 0)
    {
        hovered_axis = EGizmoAxis::None;
        renderer.SetHighlightAxis(EGizmoAxis::None);
        return;
    }

    // pick 텍스처 -> download transfer buffer 복사
    SDL_GPUDevice* raw_device = render_device->GetRawDevice();
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(raw_device);
    if (!cmd)
    {
        return;
    }

    SDL_GPUTexture* pick_texture = GetPickTexture();
    if (!pick_texture)
    {
        hovered_axis = EGizmoAxis::None;
        renderer.SetHighlightAxis(EGizmoAxis::None);
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    {
        const SDL_GPUTextureRegion src = {
            .texture = pick_texture,
            .w = 1, .h = 1, .d = 1,
        };
        const SDL_GPUTextureTransferInfo dst = {
            .transfer_buffer = pick_download_buffer,
            .offset = 0,
        };
        SDL_DownloadFromGPUTexture(copy, &src, &dst);
    }
    SDL_EndGPUCopyPass(copy);

    // Submit + fence 대기
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(raw_device, true, &fence, 1);
    SDL_ReleaseGPUFence(raw_device, fence);

    // Transfer buffer 매핑하여 pick ID 읽기
    uint32 pick_id = 0;
    if (const void* data = SDL_MapGPUTransferBuffer(raw_device, pick_download_buffer, false))
    {
        pick_id = *static_cast<const uint32*>(data);
        SDL_UnmapGPUTransferBuffer(raw_device, pick_download_buffer);
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
    const auto [editor_subsystem, entity_subsystem, viewport_subsystem, input_subsystem] =
        se::GetSubsystems<const EditorSubsystem, EntitySubsystem, const EditorViewportSubsystem, const InputSubsystem>();

    if (!editor_subsystem || !entity_subsystem || !viewport_subsystem || !input_subsystem)
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

    const auto selected_entity = editor_subsystem->GetSelection().GetPrimarySelectedEntity();
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
    if (!interaction.IsDragging() && input_subsystem->IsMouseButtonPressed(EMouseButton::Left) && hovered_axis != EGizmoAxis::None)
    {
        World& world = entity_subsystem->GetMainWorld().GetWorld();
        const auto global_tf = world.TryGetComponent<GlobalTransformComponent>(*selected_entity);
        if (!global_tf)
        {
            return;
        }

        const Vector3 center = math::TransformUtility::DecomposeTranslation(global_tf->value);
        const Quaternion rotation = (vp_info->coordinate_space == ECoordinateSpace::Local)
            ? math::TransformUtility::DecomposeRotation(global_tf->value)
            : Quaternion::Identity();

        interaction.BeginDrag(
            vp_info->gizmo_mode,
            hovered_axis,
            vp_info->coordinate_space == ECoordinateSpace::Local,
            vp_info->cursor_viewport_pos,
            center,
            rotation,
            vp_info->render_view
        );
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
            transform->position = transform->position + result.translation_delta;
            transform->dirty = true;
            break;
        case EGizmoMode::Rotate:
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
        case EGizmoMode::Scale:
            transform->scale = transform->scale + result.scale_delta;
            transform->dirty = true;
            break;
        }
    }
}
} // namespace se::editor
