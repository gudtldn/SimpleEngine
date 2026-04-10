// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEditor/Core/EditorActionSubsystem.h"

#include "SimpleEditor/Core/EditorSubsystem.h"
#include "SimpleEditor/Gizmo/GizmoSubsystem.h"
#include "SimpleEditor/UI/EditorUISubsystem.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/ChildrenComponent.h"
#include "SimpleEngine/ECS/Components/ParentComponent.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(EditorActionSubsystem)
    .DependsOn<
        EditorSubsystem,
        EditorUISubsystem,
        EditorViewportSubsystem,
        EntitySubsystem,
        GizmoSubsystem,
        InputSubsystem
    >()
    .UpdateDependsOn<
        EditorUISubsystem,
        EditorViewportSubsystem,
        GizmoSubsystem
    >();

SE_BEGIN_REFLECT(EditorActionSubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(EditorActionSubsystem)

bool EditorActionSubsystem::Initialize()
{
    editor_subsystem = &GetSubsystemChecked<EditorSubsystem>();
    entity_subsystem = &GetSubsystemChecked<EntitySubsystem>();
    gizmo_subsystem = &GetSubsystemChecked<GizmoSubsystem>();
    input_subsystem = &GetSubsystemChecked<InputSubsystem>();
    ui_subsystem = &GetSubsystemChecked<EditorUISubsystem>();
    viewport_subsystem = &GetSubsystemChecked<EditorViewportSubsystem>();
    return true;
}

void EditorActionSubsystem::Release()
{
    editor_subsystem = nullptr;
    entity_subsystem = nullptr;
    gizmo_subsystem = nullptr;
    input_subsystem = nullptr;
    ui_subsystem = nullptr;
    viewport_subsystem = nullptr;
}

void EditorActionSubsystem::Update([[maybe_unused]] double delta_time)
{
    // 카메라 조작 중에는 처리하지 않음
    if (viewport_subsystem->IsAnyCameraActive())
    {
        return;
    }

    // 텍스트 입력 중에는 처리하지 않음 (이름 변경 등)
    if (ImGui::GetIO().WantTextInput)
    {
        return;
    }

    // 뷰포트 또는 에디터 패널이 포커스 상태일 때만 처리
    const StringName focused_id = viewport_subsystem->GetFocusedViewportId();

    const bool viewport_focused = focused_id != StringName::None;
    if (!viewport_focused && !ui_subsystem->IsAnyPanelFocused())
    {
        return;
    }

    // 포커스된 뷰포트에서 작동 할 Action
    if (focused_id != StringName::None && !gizmo_subsystem->IsDragging())
    {
        // 기즈모 모드 전환
        if (input_subsystem->IsKeyPressed(EKeyCode::W))
        {
            viewport_subsystem->SetViewportGizmoMode(focused_id, EGizmoMode::Translate);
        }
        else if (input_subsystem->IsKeyPressed(EKeyCode::E))
        {
            viewport_subsystem->SetViewportGizmoMode(focused_id, EGizmoMode::Rotate);
        }
        else if (input_subsystem->IsKeyPressed(EKeyCode::R))
        {
            viewport_subsystem->SetViewportGizmoMode(focused_id, EGizmoMode::Scale);
        }

        // 기즈모 모드 순회
        else if (input_subsystem->IsKeyPressed(EKeyCode::Space))
        {
            switch (viewport_subsystem->GetViewportGizmoMode(focused_id))
            {
                case EGizmoMode::Translate:
                    viewport_subsystem->SetViewportGizmoMode(focused_id, EGizmoMode::Rotate);
                    break;
                case EGizmoMode::Rotate:
                    viewport_subsystem->SetViewportGizmoMode(focused_id, EGizmoMode::Scale);
                    break;
                case EGizmoMode::Scale:
                    viewport_subsystem->SetViewportGizmoMode(focused_id, EGizmoMode::Translate);
                    break;
                default:
                    SE_UNREACHABLE();
            }
        }

        // World / Local 변경 (Ctrl + `)
        else if (input_subsystem->HasModifier(EModifier::Ctrl) && input_subsystem->IsKeyPressed(EKeyCode::Grave))
        {
            viewport_subsystem->ToggleViewportCoordinateSpace(focused_id);
        }
    }

    // 엔티티 삭제
    if (input_subsystem->IsKeyPressed(EKeyCode::Delete))
    {
        DeleteSelectedEntities();
    }
}

void EditorActionSubsystem::DeleteEntity(Entity entity)
{
    World& world = entity_subsystem->GetMainWorld().GetWorld();
    EditorSelection& selection = editor_subsystem->GetSelection();
    DeleteEntityRecursive(world, selection, entity);
}

void EditorActionSubsystem::DeleteSelectedEntities()
{
    World& world = entity_subsystem->GetMainWorld().GetWorld();
    EditorSelection& selection = editor_subsystem->GetSelection();

    // 삭제 도중 selection이 변경되므로 먼저 복사
    const Array<Entity> to_delete = Array<Entity>::FromRange(selection.GetSelectedEntities());
    for (const Entity& entity : to_delete)
    {
        if (world.IsEntityAlive(entity))
        {
            DeleteEntityRecursive(world, selection, entity);
        }
    }
}

void EditorActionSubsystem::DeleteEntityRecursive(World& world, EditorSelection& selection, Entity entity)
{
    // 자식들을 먼저 재귀적으로 삭제
    if (const Optional children_opt = world.TryGetComponent<ChildrenComponent>(entity))
    {
        const Array<Entity> children_copy = children_opt->children;
        for (const Entity& child : children_copy)
        {
            DeleteEntityRecursive(world, selection, child);
        }
    }

    // 부모의 ChildrenComponent에서 자신을 제거
    if (const Optional parent_opt = world.TryGetComponent<ParentComponent>(entity))
    {
        if (const Optional parent_children_opt = world.TryGetComponent<ChildrenComponent>(parent_opt->parent))
        {
            parent_children_opt->children.Remove(entity);
        }
    }

    selection.DeselectEntity(entity);
    world.DestroyEntity(entity);
}
} // namespace se::editor
