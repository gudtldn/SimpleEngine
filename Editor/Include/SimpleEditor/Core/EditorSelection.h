#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/ECS/Entity.h"


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
    
    /** 에셋 브라우저에서 디렉토리를 포커싱 합니다. */
    void SetActiveContentDir(const Path& path);

    /** 에셋 브라우저에서 포커싱된 디렉토리를 가져옵니다. */
    [[nodiscard]] const Path& GetActiveContentDir() const;

private:
    HashSet<Entity> selected_entities;

    Path active_content_dir;
    // TODO: 추후 Asset 선택 추가: HashSet<Guid> selected_assets;
};
} // namespace se::editor
