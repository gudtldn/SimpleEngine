#include "UI/Panels/ImGuiDemoPanel.h"

#include "imgui.h"


namespace se::editor::ui
{
const char* ImGuiDemoPanel::GetName() const
{
    return "ImGui Demo";
}

void ImGuiDemoPanel::Draw(const EditorUIContext& context)
{
    ImGui::ShowDemoWindow(&is_visible);
}
}
