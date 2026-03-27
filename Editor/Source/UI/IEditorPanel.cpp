#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
void IEditorPanel::Draw()
{
    const bool opened = ImGui::Begin(GetName(), &is_visible, GetWindowFlags());
    is_focused = ImGui::IsWindowFocused();
    is_hovered = ImGui::IsWindowHovered();
    if (opened)
    {
        DrawContent();
    }
    ImGui::End();
}
} // namespace se::editor
