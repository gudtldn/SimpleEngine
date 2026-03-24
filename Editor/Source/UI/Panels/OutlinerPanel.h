#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se::ecs { class World; }
namespace se::editor { class EditorSelection; }

namespace se::editor
{
class OutlinerPanel : public IEditorPanel
{
public:
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    /** Entity를 트리 노드로 재귀적으로 그립니다. */
    void DrawEntityNode(ecs::World* world, EditorSelection& selection, Entity entity, Entity& entity_to_delete);

    /** Entity와 모든 자식 Entity를 재귀적으로 삭제합니다. */
    static void DeleteEntity(ecs::World* world, EditorSelection& selection, Entity entity);

    /** 이름을 바꾸려는 Entity */
    Entity renaming_entity;

    /** 바꿀 이름 */
    String rename_name;

    /** 다음 프레임에 이름 편집 모드로 바꿀지 여부 */
    bool rename_focus_pending = false;
};
} // namespace se::editor
