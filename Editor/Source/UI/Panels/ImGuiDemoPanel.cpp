#include "UI/Panels/ImGuiDemoPanel.h"

#include "imgui.h"


namespace se::editor::ui
{
ImGuiDemoPanel::ImGuiDemoPanel()
{
    is_visible = false;
}

const char* ImGuiDemoPanel::GetName() const
{
    return "ImGui Demo";
}

void ImGuiDemoPanel::Draw()
{
    ImGui::ShowDemoWindow(&is_visible);
}
}
