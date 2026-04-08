#include "SimpleEditor/Gizmo/GizmoSubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
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
} // namespace se::editor
