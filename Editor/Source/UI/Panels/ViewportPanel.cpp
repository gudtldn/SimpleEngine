#include "UI/Panels/ViewportPanel.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
ViewportPanel::ViewportPanel(const StringName& in_viewport_id)
    : viewport_id(in_viewport_id)
{
}

const char* ViewportPanel::GetName() const
{
    return viewport_id.CStr();
}

void ViewportPanel::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(GetName(), &is_visible);
    {
        if (EditorViewportSubsystem* viewport_sys = GetSubsystem<EditorViewportSubsystem>())
        {
            const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
            const uint32 width = static_cast<uint32>(viewport_size.x);
            const uint32 height = static_cast<uint32>(viewport_size.y);

            if (SDL_GPUTexture* texture_to_draw = viewport_sys->UpdateAndGetViewportTexture(viewport_id, width, height))
            {
                ImGui::Image(texture_to_draw, viewport_size);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
} // namespace se::editor
