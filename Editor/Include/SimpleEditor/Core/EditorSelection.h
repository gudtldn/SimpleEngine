#pragma once
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/ECS/Entity.h"

#include <editor_export.h>


namespace se::editor
{
class SE_EDITOR_API EditorSelection
{
public:
    // 엔티티 선택 관리
    void SelectEntity(Entity entity, bool clear_others = true);
    void DeselectEntity(Entity entity);
    void ClearSelection();

    [[nodiscard]] bool IsSelected(Entity entity) const;
    [[nodiscard]] const HashSet<Entity>& GetSelectedEntities() const;

    /** 마지막으로 선택된 Entity */
    [[nodiscard]] Optional<Entity> GetPrimarySelectedEntity() const;

private:
    HashSet<Entity> selected_entities;
    // TODO: 추후 Asset 선택 추가: HashSet<Guid> selected_assets;
};
} // namespace se::editor
