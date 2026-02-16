#include "UI/Panels/DetailPanel.h"

#include "SimpleEditor/Core/EditorSubsystem.h"
#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ComponentRegistry.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
DetailPanel::DetailPanel()
    : components{
        decltype(components)::FromRange(TypeRegistry::Get().GetAllTypes().Values() | std::views::filter([](const TypeInfo& info) -> bool
        {
            return info.flags.IsAnySet(ETypeFlags::IsComponent);
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
    SE_SCOPE_DEFER{ ImGui::End(); };

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
        static TypeId selected_id;
        const Entity& entity = selection.GetPrimarySelectedEntity().Value();

        ImGui::Separator();

        const ImVec2 size = ImVec2(0, 5 * ImGui::GetTextLineHeightWithSpacing());
        if (ImGui::BeginListBox("##Component Lists", size))
        {
            SE_SCOPE_DEFER{ ImGui::EndListBox(); };

            for (const TypeInfo& component_info : components)
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
        }

        if (ImGui::BeginCombo("Add Component", "Select Component"))
        {
            SE_SCOPE_DEFER{ ImGui::EndCombo(); };

            for (const TypeInfo& component_info : components)
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
        }

        ImGui::Separator();

        // 선택된 Component의 프로퍼티 렌더링
        if (selected_id.IsValid())
        {
            if (const Optional type_info_opt = TypeRegistry::Get().Find(selected_id))
            {
                ecs::World* world = world_subsystem->GetWorld();
                if (const Optional interface_opt = ecs::ComponentRegistry::Get().GetInterface(selected_id))
                {
                    if (void* component_data = interface_opt->get_component_mutable(*world, entity))
                    {
                        const StringView& view = type_info_opt->name;
                        ImGui::Text("[%.*s]", static_cast<int>(view.ByteLen()), view.Data());
                        ImGui::Separator();
                        DrawerRegistry::Get().DrawProperties(*type_info_opt, component_data);
                    }
                }
            }
        }
    }
}
}  // namespace se::editor
