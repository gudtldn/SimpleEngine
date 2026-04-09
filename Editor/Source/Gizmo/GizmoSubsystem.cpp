#include "SimpleEditor/Gizmo/GizmoSubsystem.h"

#include "SimpleEditor/Core/EditorSubsystem.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Math/TransformUtility.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(GizmoSubsystem)
    .DependsOn<RenderSubsystem>();

SE_BEGIN_REFLECT(GizmoSubsystem, meta::Internal)
SE_END_REFLECT(GizmoSubsystem)

bool GizmoSubsystem::Initialize()
{
    graphics::RenderDevice& render_device = GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();
    draw_list = std::make_unique<GizmoDrawList>(render_device);
    return true;
}

void GizmoSubsystem::Release()
{
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

    const Vector3 position = math::TransformUtility::DecomposeTranslation(global_tf->value);
    const Quaternion rotation = (vp_info->coordinate_space == ECoordinateSpace::Local)
        ? math::TransformUtility::DecomposeRotation(global_tf->value)
        : Quaternion::Identity();

    renderer.SetMode(vp_info->gizmo_mode);
    renderer.Draw(*draw_list, position, rotation, vp_info->render_view);
}
} // namespace se::editor
