#include "UI/Panels/OutlinerPanel.h"

#include "UI/EditorUISubsystem.h"
#include "SimpleEngine/World/Query.h"
#include "SimpleEngine/World/WorldSubsystem.h"

#include "imgui.h"

namespace se::editor::ui
{
const char* OutlinerPanel::GetName() const
{
    return "Outliner";
}

void OutlinerPanel::Draw(EditorUIContext& context)
{
    ImGui::Begin(GetName(), &is_visible);
    {
        WorldSubsystem* world_subsystem = utility::GetSubsystemUnchecked<WorldSubsystem>();
        if (!world_subsystem)
        {
            return;
        }

        world::World* world = world_subsystem->GetWorld();
        world::Query query = world->QueryEntities<world::Entity>();

        for (const auto& [entity] : query)
        {
            if (ImGui::Selectable(String::Format("Entity {}", entity.GetId()).CStr(), context.selected_entity == entity))
            {
                // 엔티티를 클릭하면 EditorContext의 선택된 엔티티를 업데이트
                context.selected_entity = entity;
            }
        }
    }
    ImGui::End();
}
}
