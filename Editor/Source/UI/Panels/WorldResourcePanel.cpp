#include "UI/Panels/WorldResourcePanel.h"

#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "../../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ECSRegistry.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
const char* WorldResourcePanel::GetName() const
{
    return "World Resource";
}

void WorldResourcePanel::DrawContent()
{
    EntitySubsystem* entity_subsystem = GetSubsystem<EntitySubsystem>();
    if (!entity_subsystem)
    {
        return;
    }

    for (auto& [world_name, ctx] : entity_subsystem->GetWorlds())
    {
        ImGui::PushID(world_name.CStr());
        SE_SCOPE_DEFER{ ImGui::PopID(); };

        const String world_header = String::Format("[{}]", world_name.CStr());
        if (!ImGui::CollapsingHeader(world_header.CStr(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            continue;
        }

        World& world = ctx.GetWorld();
        for (const auto& [res_type, ops] : ECSRegistry::Get().GetResourceOpsMap())
        {
            void* resource = ops.get_resource_mutable(world);
            if (!resource)
            {
                continue;
            }

            const auto type_info = TypeRegistry::Get().Find(res_type);
            if (!type_info || type_info->annotations.Has<display::HiddenAnnotation>())
            {
                continue;
            }

            const String res_header = type_info->name;

            ImGui::PushID(res_header.CStr());
            SE_SCOPE_DEFER{ ImGui::PopID(); };

            if (ImGui::TreeNodeEx(res_header.CStr(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (const auto legacy_info = TypeRegistry_v1::Get().Find(TypeId_v1::FromName(StringName{ type_info->name })))
                {
                    DrawerRegistry::Get().DrawProperties(*legacy_info, resource);
                }
                ImGui::TreePop();
            }
        }
    }
}
} // namespace se::editor
