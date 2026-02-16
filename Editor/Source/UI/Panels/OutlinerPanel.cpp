#include "UI/Panels/OutlinerPanel.h"

#include "SimpleEditor/Core/EditorSubsystem.h"
#include "SimpleEditor/UI/EditorUISubsystem.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
const char* OutlinerPanel::GetName() const
{
    return "Outliner";
}

void OutlinerPanel::Draw()
{
    ImGui::Begin(GetName(), &is_visible);
    {
        const auto [world_subsystem, editor_subsystem] = GetSubsystems<const WorldSubsystem, EditorSubsystem>();
        if (world_subsystem && editor_subsystem)
        {
            ecs::World* world = world_subsystem->GetWorld();
            EditorSelection& selection = editor_subsystem->GetSelection();

            for (const auto& [entity] : world->QueryEntities<Entity>())
            {
                const String label = String::Format("Entity {}", entity.GetId());
                const bool is_selected = selection.IsSelected(entity);
                if (ImGui::Selectable(label.CStr(), is_selected))
                {
                    if (ImGui::GetIO().KeyCtrl)
                    {
                        if (is_selected)
                        {
                            selection.DeselectEntity(entity);
                        }
                        else
                        {
                            selection.SelectEntity(entity, false); // false = clear others 안함
                        }
                    }
                    else
                    {
                        selection.SelectEntity(entity, true); // true = 나머지는 해제
                    }
                }
            }
        }
        else
        {
            ConsoleLogOnce(ELogLevel::Error, "EditorSubsystem or WorldSubsystem is not initialized!");
        }
    }
    ImGui::End();
}
} // namespace se::editor
