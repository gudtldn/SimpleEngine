#include "UI/Panels/ViewportPanel.h"

#include "imgui.h"


namespace se::editor::ui
{
const char* ViewportPanel::GetName() const
{
    return "Viewport";
}

void ViewportPanel::Draw(EditorUIContext& context)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(GetName(), &is_visible, ImGuiWindowFlags_NoBackground);
    {

    }
    ImGui::End();
    ImGui::PopStyleVar();
}
}
