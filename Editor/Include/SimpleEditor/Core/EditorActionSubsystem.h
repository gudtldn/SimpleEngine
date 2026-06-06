#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se
{
class EntitySubsystem;
class InputSubsystem;
class World;
}

namespace se::editor
{
class EditorSelection;
class EditorViewportSubsystem;
class SelectionSubsystem;

/**
 * 에디터 전반에서 공유되는 Entity 조작 액션을 처리하는 Subsystem
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) EditorActionSubsystem : public SubsystemBase, public IUpdatable
{
    SE_CLASS(EditorActionSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void Update(f64 delta_time) override;
    //~ End IUpdatable

public:
    /** 특정 Entity 하나를 자식 계층 포함하여 삭제합니다. */
    void DeleteEntity(Entity entity);

    /** EditorSelection에 선택된 모든 Entity를 삭제합니다. */
    void DeleteSelectedEntities();

private:
    static void DeleteEntityRecursive(World& world, EditorSelection& selection, Entity entity);

private:
    SelectionSubsystem* selection_subsystem = nullptr;
    EditorViewportSubsystem* viewport_subsystem = nullptr;
    EntitySubsystem* entity_subsystem = nullptr;
    InputSubsystem* input_subsystem = nullptr;
};
} // namespace se::editor
