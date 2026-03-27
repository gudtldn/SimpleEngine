// NOLINTBEGIN(*-isolate-declaration)

#include "UI/Panels/CameraPanel.h"

#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"
#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"


namespace se::editor
{
using namespace se::math;

const char* CameraPanel::GetName() const
{
    return "EditorCamera";
}

void CameraPanel::DrawContent()
{
    EditorViewportSubsystem* viewport_subsystem = GetSubsystem<EditorViewportSubsystem>();
    if (!viewport_subsystem)
    {
        return;
    }

    for (auto& [viewport_id, camera] : viewport_subsystem->GetViewportCameras())
    {
        if (ImGui::TreeNodeEx(viewport_id.CStr(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<decltype(camera)>());
            DrawerRegistry::Get().DrawProperties(info, &camera);
            ImGui::TreePop();
        }
    }
}
} // namespace se::editor

// NOLINTEND(*-isolate-declaration)
