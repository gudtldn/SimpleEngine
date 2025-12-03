#include "UI/Panels/OutlinerPanel.h"

#include "UI/EditorUISubsystem.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor::ui
{
const char* OutlinerPanel::GetName() const
{
    return "Outliner";
}

void OutlinerPanel::Draw()
{
    ImGui::Begin(GetName(), &is_visible);
    {
        WorldSubsystem* world_subsystem = GetSubsystem<WorldSubsystem>();
        if (!world_subsystem)
        {
            return;
        }

        ecs::World* world = world_subsystem->GetWorld();
        ecs::Query query = world->QueryEntities<Entity>();

        for (const auto& [entity] : query)
        {
            static Optional<Entity> selected_entity; // TODO: 이거 수정해야함. 임시코드
            if (ImGui::Selectable(String::Format("Entity {}", entity.GetId()).CStr(), selected_entity == entity))
            {
                // 엔티티를 클릭하면 EditorContext의 선택된 엔티티를 업데이트
                selected_entity = entity;
            }
        }
    }
    ImGui::End();
}
}
