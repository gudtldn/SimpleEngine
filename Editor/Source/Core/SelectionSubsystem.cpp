#include "SimpleEditor/Core/SelectionSubsystem.h"

#include "SimpleEditor/Gizmo/GizmoSubsystem.h"
#include "SimpleEditor/Picking/PickSubsystem.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Input/MouseButton.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/ECS/EntityPickId.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/ParentComponent.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(SelectionSubsystem)
    .DependsOn<
        EditorViewportSubsystem,
        InputSubsystem,
        PickSubsystem,
        RenderSubsystem
    >()
    .UpdateDependsOn<GizmoSubsystem>();

SE_BEGIN_REFLECT(SelectionSubsystem, meta::Reflect, meta::Hidden, meta::Transient)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(SelectionSubsystem)

bool SelectionSubsystem::Initialize()
{
    input_subsystem = &GetSubsystemChecked<InputSubsystem>();
    entity_subsystem = &GetSubsystemChecked<EntitySubsystem>();
    viewport_subsystem = &GetSubsystemChecked<EditorViewportSubsystem>();
    gizmo_subsystem = &GetSubsystemChecked<GizmoSubsystem>();
    pick_subsystem = &GetSubsystemChecked<PickSubsystem>();
    return true;
}

void SelectionSubsystem::Release()
{
    input_subsystem = nullptr;
    entity_subsystem = nullptr;
    viewport_subsystem = nullptr;
    gizmo_subsystem = nullptr;
    pick_subsystem = nullptr;
}

void SelectionSubsystem::Update(f64 /*delta_time*/)
{
    // 카메라 조작 중에는 선택 처리하지 않음
    if (viewport_subsystem->IsAnyCameraActive())
    {
        return;
    }

    // 기즈모 드래그 중에는 선택 처리하지 않음
    if (gizmo_subsystem->IsDragging())
    {
        return;
    }

    // 좌클릭 시에만
    if (!input_subsystem->IsMouseButtonPressed(EMouseButton::Left))
    {
        return;
    }

    // 기즈모 축에 호버 중이면 기즈모 인터랙션 -> 선택 로직 건너뜀
    if (gizmo_subsystem->GetHoveredAxis() != EGizmoAxis::None)
    {
        return;
    }

    // 호버된 뷰포트가 없으면 처리하지 않음
    if (!viewport_subsystem->GetHoveredViewportInfo().HasValue())
    {
        return;
    }

    const EntityPickId pick_id = pick_subsystem->GetPickId();
    if (const auto decoded_id = pick_id.Decode())
    {
        World& world = entity_subsystem->GetMainWorld().GetWorld();
        if (const auto resolved = world.TryResolveEntity(*decoded_id))
        {
            // pick된 entity(메시 자식)로부터 root entity까지 부모 체인 탐색
            // TODO: drill-down 구현 - root가 이미 선택된 상태에서 재클릭 시 자식 선택
            Entity target = *resolved;
            while (const auto parent = world.TryGetComponent<ParentComponent>(target))
            {
                if (!world.IsEntityAlive(parent->parent))
                {
                    break;
                }
                target = parent->parent;
            }

            const bool clear_others = !input_subsystem->HasModifier(EModifier::Ctrl);
            selection.SelectEntity(target, clear_others);
        }
        else
        {
            // stale entity id (이미 삭제됨) -> 빈 공간 클릭과 동일 처리
            selection.ClearSelection();
        }
    }
    else
    {
        // 빈 공간 클릭 -> 선택 해제
        selection.ClearSelection();
    }
}
} // namespace se::editor
