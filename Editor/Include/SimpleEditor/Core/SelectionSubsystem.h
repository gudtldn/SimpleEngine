#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Core/EditorSelection.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"
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
class SE_EDITOR_API SelectionSubsystem : public SubsystemBase, public IUpdatable
{
    friend struct ::se::Registrar<SelectionSubsystem>;

public:
    SE_RTTI(SelectionSubsystem)

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
    SE_ANNOTATE(selection, Ignore)
    EditorSelection selection;

    SE_ANNOTATE(input_subsystem, Ignore)
    InputSubsystem* input_subsystem = nullptr;
    SE_ANNOTATE(entity_subsystem, Ignore)
    EntitySubsystem* entity_subsystem = nullptr;
    SE_ANNOTATE(viewport_subsystem, Ignore)
    EditorViewportSubsystem* viewport_subsystem = nullptr;
    SE_ANNOTATE(gizmo_subsystem, Ignore)
    GizmoSubsystem* gizmo_subsystem = nullptr;
    SE_ANNOTATE(pick_subsystem, Ignore)
    PickSubsystem* pick_subsystem = nullptr;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::SelectionSubsystem, SE_EDITOR_API)
