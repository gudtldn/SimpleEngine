// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEditor/Core/EditorActionSubsystem.h"

#include "SimpleEditor/Core/SelectionSubsystem.h"
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
        SelectionSubsystem,
        EditorViewportSubsystem,
        EntitySubsystem,
        InputSubsystem
    >();

SE_BEGIN_REFLECT(EditorActionSubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(EditorActionSubsystem)

bool EditorActionSubsystem::Initialize()
{
    selection_subsystem = &GetSubsystemChecked<SelectionSubsystem>();
    viewport_subsystem = &GetSubsystemChecked<EditorViewportSubsystem>();
    entity_subsystem = &GetSubsystemChecked<EntitySubsystem>();
    input_subsystem = &GetSubsystemChecked<InputSubsystem>();
    return true;
}

void EditorActionSubsystem::Release()
{
    selection_subsystem = nullptr;
    viewport_subsystem = nullptr;
    entity_subsystem = nullptr;
    input_subsystem = nullptr;
}

void EditorActionSubsystem::Update(double /*delta_time*/)
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

    // 에디터 윈도우가 포커스 상태가 아니면 처리하지 않음
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
    {
        return;
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
    EditorSelection& selection = selection_subsystem->GetSelection();
    DeleteEntityRecursive(world, selection, entity);
}

void EditorActionSubsystem::DeleteSelectedEntities()
{
    World& world = entity_subsystem->GetMainWorld().GetWorld();
    EditorSelection& selection = selection_subsystem->GetSelection();

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
