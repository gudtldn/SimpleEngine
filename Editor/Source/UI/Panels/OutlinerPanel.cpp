#include "UI/Panels/OutlinerPanel.h"
#include "UI/ImGui/ImGuiString.h"

#include "SimpleEditor/Core/EditorSubsystem.h"
#include "SimpleEditor/UI/EditorUISubsystem.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/ECS/Components/ChildrenComponent.h"
#include "SimpleEngine/ECS/Components/NameComponent.h"
#include "SimpleEngine/ECS/Components/ParentComponent.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
const char* OutlinerPanel::GetName() const
{
    return "Outliner";
}

void OutlinerPanel::Draw()
{
    ImGui::Begin(GetName(), &is_visible);
    SE_SCOPE_DEFER{ ImGui::End(); };

    const auto [world_subsystem, editor_subsystem] = GetSubsystems<const WorldSubsystem, EditorSubsystem>();
    if (!(world_subsystem && editor_subsystem))
    {
        ConsoleLogOnce(ELogLevel::Error, "EditorSubsystem or WorldSubsystem is not initialized!");
        return;
    }

    ecs::World* world = world_subsystem->GetWorld();
    EditorSelection& selection = editor_subsystem->GetSelection();

    Entity entity_to_delete;

    for (const auto& [entity, name_opt] : world->QueryEntities<Entity, Optional<NameComponent&>>())
    {
        const String display_name = (name_opt && !name_opt->name.IsEmpty())
            ? name_opt->name
            : String::Format("Entity {}", entity.GetId());

        ImGui::PushID(static_cast<int>(entity.GetId()));

        if (renaming_entity == entity)
        {
            // 이름 편집 모드
            if (rename_focus_pending)
            {
                ImGui::SetKeyboardFocusHere();
                rename_focus_pending = false;
            }

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##rename", &rename_name, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (name_opt)
                {
                    name_opt->name = rename_name;
                }
                else
                {
                    world->AddComponent(entity, NameComponent{ .name = rename_name });
                }
                renaming_entity = Entity{};
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                renaming_entity = Entity{};
            }
        }
        else
        {
            const bool is_selected = selection.IsSelected(entity);
            if (ImGui::Selectable(display_name.CStr(), is_selected))
            {
                if (ImGui::GetIO().KeyCtrl)
                {
                    if (is_selected)
                    {
                        selection.DeselectEntity(entity);
                    }
                    else
                    {
                        selection.SelectEntity(entity, false);
                    }
                }
                else
                {
                    selection.SelectEntity(entity, true);
                }
            }

            // 우클릭 컨텍스트 메뉴
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    renaming_entity = entity;
                    rename_name = display_name;
                    rename_focus_pending = true;
                }

                if (ImGui::MenuItem("Delete Entity"))
                {
                    entity_to_delete = entity;
                }
                ImGui::EndPopup();
            }
        }

        ImGui::PopID();
    }

    // 순회 완료 후 삭제 처리
    if (entity_to_delete.IsValid())
    {
        if (renaming_entity == entity_to_delete)
        {
            renaming_entity = Entity{};
        }
        DeleteEntity(world, selection, entity_to_delete);
    }
}

void OutlinerPanel::DeleteEntity(ecs::World* world, EditorSelection& selection, Entity entity)
{
    // 자식들을 먼저 재귀적으로 삭제
    if (const Optional children_opt = world->TryGetComponent<ChildrenComponent>(entity))
    {
        const Array<Entity> children_copy = children_opt->children;
        for (const Entity& child : children_copy)
        {
            DeleteEntity(world, selection, child);
        }
    }

    // 부모의 ChildrenComponent에서 자신을 제거
    if (const Optional parent_opt = world->TryGetComponent<ParentComponent>(entity))
    {
        if (const Optional parent_children_opt = world->TryGetComponent<ChildrenComponent>(parent_opt->parent))
        {
            parent_children_opt->children.Remove(entity);
        }
    }

    selection.DeselectEntity(entity);
    world->DestroyEntity(entity);
}
} // namespace se::editor
