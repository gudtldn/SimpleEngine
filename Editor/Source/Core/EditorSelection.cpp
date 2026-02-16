#include "SimpleEditor/Core/EditorSelection.h"


namespace se::editor
{
void EditorSelection::SelectEntity(Entity entity, bool clear_others)
{
    if (clear_others)
    {
        ClearSelection();
    }
    selected_entities.Emplace(entity);
}

void EditorSelection::DeselectEntity(Entity entity)
{
    selected_entities.Remove(entity);
}

void EditorSelection::ClearSelection()
{
    selected_entities.Clear();
}

bool EditorSelection::IsSelected(Entity entity) const
{
    return selected_entities.Contains(entity);
}

const HashSet<Entity>& EditorSelection::GetSelectedEntities() const
{
    return selected_entities;
}

Optional<Entity> EditorSelection::GetPrimarySelectedEntity() const
{
    if (selected_entities.IsEmpty())
    {
        return std::nullopt;
    }
    // HashSet이라 순서가 보장되진 않지만, 단일 선택 모드에서는 유효함
    return *selected_entities.begin();
}
} // namespace se::editor
