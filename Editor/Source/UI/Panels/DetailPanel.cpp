#include "UI/Panels/DetailPanel.h"

#include "imgui.h"


namespace se::editor::ui
{
const char* DetailPanel::GetName() const
{
    return "Detail";
}

void DetailPanel::Draw(EditorUIContext& context)
{
    ImGui::Begin(GetName(), &is_visible);
    {

    }
    ImGui::End();
}
}
