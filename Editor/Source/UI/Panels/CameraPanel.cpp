// NOLINTBEGIN(*-isolate-declaration)

#include "UI/Panels/CameraPanel.h"

#include "SimpleEditor/UI/EditorViewportSubsystem.h"
#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"

#include <ranges>


namespace se::editor
{
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

    auto valid_cameras_view = viewport_subsystem->GetViewports()
        | std::views::keys
        | std::views::transform([&](const StringName& id)
        {
            return std::pair{ id, viewport_subsystem->GetViewportCamera(id) };
        })
        | std::views::filter([](const auto& pair) { return pair.second.HasValue(); })
        | std::views::transform([](auto&& pair)
        {
            return std::pair<const StringName&, EditorCameraState&>{ pair.first, *pair.second };
        });

    for (auto [viewport_id, camera] : valid_cameras_view)
    {
        if (ImGui::TreeNodeEx(viewport_id.CStr(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Of<decltype(camera)>());
            DrawerRegistry::Get().DrawProperties(info, &camera);
            ImGui::TreePop();
        }
    }
}
} // namespace se::editor

// NOLINTEND(*-isolate-declaration)
