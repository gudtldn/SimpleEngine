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


    // ParentComponent가 없는 루트 엔티티를 최상위에서 렌더링
    for (const auto& [entity] : world->QueryEntities<Entity, ecs::Without<ParentComponent>>())
    {
        DrawEntityNode(world, selection, entity, entity_to_delete);
    }

    // ParentComponent가 있으나 부모가 이미 소멸된 고아 엔티티를 루트로 렌더링
    for (const auto& [entity, parent_comp] : world->QueryEntities<Entity, ParentComponent&>())
    {
        if (!world->IsEntityAlive(parent_comp.parent))
        {
            DrawEntityNode(world, selection, entity, entity_to_delete);
        }
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

void OutlinerPanel::DrawEntityNode(ecs::World* world, EditorSelection& selection, Entity entity, Entity& entity_to_delete)
{
    const Optional name_opt = world->TryGetComponent<NameComponent>(entity);
    const Optional children_opt = world->TryGetComponent<ChildrenComponent>(entity);
    const bool has_children = children_opt && !children_opt->children.IsEmpty();

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
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
        if (!has_children)
        {
            // 자식이 없는 경우 펼침 화살표 없이 선택만 가능한 리프 노드로 표시
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        if (selection.IsSelected(entity))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool node_open = ImGui::TreeNodeEx(display_name.CStr(), flags);

        // OpenOnArrow 플래그로 인해 화살표 클릭은 토글, 레이블 클릭은 선택으로 분리된다
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            if (ImGui::GetIO().KeyCtrl)
            {
                if (selection.IsSelected(entity))
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

        // 자식이 있는 노드가 열려있을 때만 재귀적으로 자식을 렌더링
        if (node_open && has_children)
        {
            for (const Entity& child : children_opt->children)
            {
                if (world->IsEntityAlive(child))
                {
                    DrawEntityNode(world, selection, child, entity_to_delete);
                }
            }
            ImGui::TreePop();
        }
    }

    ImGui::PopID();
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
