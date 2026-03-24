// NOLINTBEGIN(*-isolate-declaration)

#include "UI/Panels/CameraPanel.h"

#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
using namespace se::math;

const char* CameraPanel::GetName() const
{
    return "EditorCamera";
}

void CameraPanel::Draw()
{
    ImGui::Begin(GetName(), &is_visible);
    SE_SCOPE_DEFER{ ImGui::End(); };

    EditorViewportSubsystem* viewport_subsystem = GetSubsystem<EditorViewportSubsystem>();
    if (!viewport_subsystem)
    {
        return;
    }

    for (auto& [viewport_id, camera] : viewport_subsystem->GetViewportCameras())
    {
        const bool open = ImGui::TreeNodeEx(viewport_id.CStr(), ImGuiTreeNodeFlags_DefaultOpen);
        if (!open)
        {
            continue;
        }
        SE_SCOPE_DEFER{ ImGui::TreePop(); };

        // Position
        ImGui::DragScalarN("Position", ImGuiDataType_Double, &camera.position.x, 3, 0.05f);

        // Rotation (Euler degrees)
        ImGui::DragScalarN("Rotation", ImGuiDataType_Double, &camera.rotation.pitch.value, 3, 0.5f);

        // FOV
        constexpr double min_fov = 0.0, max_fov = 180.0;
        ImGui::DragScalarN("FOV", ImGuiDataType_Double, &camera.fov_y.value, 1, 0.5f, &min_fov, &max_fov);

        // Near / Far
        constexpr double min_near = 0.001, max_near = 100.0;
        constexpr double min_far = 1.0, max_far = 100000.0;

        ImGui::DragScalarN("Near", ImGuiDataType_Double, &camera.near_plane, 1, 0.001f, &min_near, &max_near);
        ImGui::DragScalarN("Far", ImGuiDataType_Double, &camera.far_plane, 1, 1.0f, &min_far, &max_far);

        // Move Speed / Look Sensitivity
        constexpr double min_speed = 0.01, max_speed = 1000.0;
        constexpr double min_sens = 0.001, max_sens = 10.0;

        ImGui::DragScalarN("Move Speed", ImGuiDataType_Double, &camera.move_speed, 1, 0.01f, &min_speed, &max_speed);
        ImGui::DragScalarN("Look Sensitivity", ImGuiDataType_Double, &camera.look_sensitivity, 1, 0.001f, &min_sens, &max_sens);
    }
}
} // namespace se::editor

// NOLINTEND(*-isolate-declaration)
