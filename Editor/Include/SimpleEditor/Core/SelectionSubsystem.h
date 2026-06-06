#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Core/EditorSelection.h"

#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se
{
class InputSubsystem;
class EntitySubsystem;
} // namespace se

namespace se::editor
{
class EditorViewportSubsystem;
class GizmoSubsystem;
class PickSubsystem;

/**
 * Entity 선택 상태를 관리하고, 뷰포트 클릭을 통한 Entity 선택을 처리하는 Subsystem
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) SelectionSubsystem : public SubsystemBase, public IUpdatable
{
    SE_CLASS(SelectionSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void Update(f64 delta_time) override;
    //~ End IUpdatable

public:
    [[nodiscard]] EditorSelection& GetSelection() { return selection; }
    [[nodiscard]] const EditorSelection& GetSelection() const { return selection; }

private:
    EditorSelection selection;

    InputSubsystem* input_subsystem = nullptr;
    EntitySubsystem* entity_subsystem = nullptr;
    EditorViewportSubsystem* viewport_subsystem = nullptr;
    GizmoSubsystem* gizmo_subsystem = nullptr;
    PickSubsystem* pick_subsystem = nullptr;
};
} // namespace se::editor
