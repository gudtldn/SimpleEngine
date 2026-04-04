#include "UI/Panels/WorldResourcePanel.h"

#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

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

            const Optional<const TypeInfo&> type_info = TypeRegistry::Get().Find(res_type);
            if (!type_info || type_info->flags.IsAnySet(ETypeFlags::Hidden))
            {
                continue;
            }

            const String res_header = type_info->name;

            ImGui::PushID(res_header.CStr());
            SE_SCOPE_DEFER{ ImGui::PopID(); };

            if (ImGui::TreeNodeEx(res_header.CStr(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawerRegistry::Get().DrawProperties(*type_info, resource);
                ImGui::TreePop();
            }
        }
    }
}
} // namespace se::editor
