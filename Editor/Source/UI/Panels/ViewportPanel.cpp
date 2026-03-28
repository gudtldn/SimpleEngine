#include "UI/Panels/ViewportPanel.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
ViewportPanel::ViewportPanel(const StringName& in_viewport_id, bool default_visibility)
    : viewport_id(in_viewport_id)
{
    is_visible = default_visibility;
}

const char* ViewportPanel::GetName() const
{
    return viewport_id.CStr();
}

void ViewportPanel::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(GetName(), &is_visible);
    is_focused = ImGui::IsWindowFocused();
    is_hovered = ImGui::IsWindowHovered();
    {
        if (EditorViewportSubsystem* viewport_sys = GetSubsystem<EditorViewportSubsystem>())
        {
            viewport_sys->UpdateViewportFocus(viewport_id, ImGui::IsWindowFocused(), ImGui::IsWindowHovered());

            const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
            const uint32 width = static_cast<uint32>(viewport_size.x);
            const uint32 height = static_cast<uint32>(viewport_size.y);

            // 화면 크기 업데이트
            viewport_sys->UpdateViewportSize(viewport_id, width, height);

            if (void* texture_to_draw = viewport_sys->GetViewportTextureID(viewport_id))
            {
                ImGui::Image(texture_to_draw, viewport_size);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
} // namespace se::editor
