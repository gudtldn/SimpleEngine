#include "UI/Panels/DetailPanel.h"

#include "Core/EditorSubsystem.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Reflection/TypeRegistry.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor::ui
{
DetailPanel::DetailPanel()
    : components{
        decltype(components)::FromRange(refl::TypeRegistry::Get().GetAllTypes().Values() | std::views::filter([](const refl::TypeInfo& info) -> bool
        {
            return info.flags.IsAnySet(refl::ETypeFlags::Component);
        }))
    }
{
}

const char* DetailPanel::GetName() const
{
    return "Detail";
}

void DetailPanel::Draw()
{
    ImGui::Begin(GetName(), &is_visible);
    [&]{
        const auto [world_subsystem, editor_subsystem] = GetSubsystems<const WorldSubsystem, EditorSubsystem>();
        if (!(world_subsystem && editor_subsystem))
        {
            return;
        }

        const EditorSelection& selection = editor_subsystem->GetSelection();
        const HashSet<Entity>& selected_entities = selection.GetSelectedEntities();

        ImGui::Text("Selected Entities Count: %llu", selected_entities.Len());
        for (const Entity& entity : selected_entities)
        {
            ImGui::Text("- %s", String::Format("Entity {}", entity.GetId()).CStr());
        }

        if (selected_entities.Len() == 1)
        {
            static refl::TypeId selected_id;
            const Entity& entity = selection.GetPrimarySelectedEntity().Value();

            ImGui::Separator();

            const ImVec2 size = ImVec2(0, 5 * ImGui::GetTextLineHeightWithSpacing());
            if (ImGui::BeginListBox("##Component Lists", size))
            {
                for (const refl::TypeInfo& component_info : components)
                {
                    ecs::World* world = world_subsystem->GetWorld();
                    ecs::IStorage* storage = world->GetStorage(component_info.type_id);
                    if (storage && storage->Contains(entity))
                    {
                        const String label = component_info.name;
                        if (ImGui::Selectable(label.CStr(), selected_id == component_info.type_id))
                        {
                            selected_id = component_info.type_id;
                        }
                    }
                }
                ImGui::EndListBox();
            }

            if (ImGui::BeginCombo("Add Component", "Select Component"))
            {
                for (const refl::TypeInfo& component_info : components)
                {
                    ecs::World* world = world_subsystem->GetWorld();
                    if (ecs::IStorage* storage = world->GetStorage(component_info.type_id))
                    {
                        const String label = component_info.name;
                        if (ImGui::Selectable(label.CStr(), false))
                        {
                            if (!storage->Contains(entity))
                            {
                                storage->EmplaceDefault(entity);
                            }
                        }
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            // TODO: 선택된 Component 정보 띄우기
        }
    }();
    ImGui::End();
}
}  // namespace se::editor::ui
