#include "UI/Panels/DetailPanel.h"

#include "SimpleEditor/Core/EditorSubsystem.h"
#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ComponentRegistry.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


namespace se::editor
{
using namespace se::math;

DetailPanel::DetailPanel()
    : components{ // TODO: 여기 ComponentRegistry 순회하는걸로 최적화 가능 | 아니지, 그냥 ComponentRegistry.Keys()로 하면 되지않나?
        decltype(components)::FromRange(TypeRegistry::Get().GetAllTypes().Values() | std::views::filter([](const TypeInfo& info) -> bool
        {
            return info.flags.IsAnySet(ETypeFlags::IsComponent);
        }))
    }
{
}

const char* DetailPanel::GetName() const
{
    return "Detail";
}

void DetailPanel::Draw()
{
    ImGui::Begin(GetName(), &is_visible);
    SE_SCOPE_DEFER{ ImGui::End(); };

    const auto [world_subsystem, editor_subsystem] = GetSubsystems<const WorldSubsystem, EditorSubsystem>();
    if (!(world_subsystem && editor_subsystem))
    {
        return;
    }

    const EditorSelection& selection = editor_subsystem->GetSelection();
    const HashSet<Entity>& selected_entities = selection.GetSelectedEntities();

    ImGui::Text("Selected Entities Count: %llu", selected_entities.Len());
    for (const Entity& entity : selected_entities)
    {
        ImGui::Text("- %s", String::Format("Entity {}", entity.GetId()).CStr());
    }

    // Entity가 다중으로 선택되어 있으면 Property 정보를 그리지 않음
    if (selected_entities.Len() != 1)
    {
        return;
    }

    const Entity& entity = selection.GetPrimarySelectedEntity().Value();
    ecs::World* world = world_subsystem->GetWorld();

    // 선택된 Entity가 바뀌면 컴포넌트 선택 초기화
    if (last_selected_entity != entity)
    {
        selected_component_id = TypeId{};
        last_selected_entity = entity;
    }

    ImGui::Separator();

    // 컴포넌트 리스트
    TypeId component_to_remove;
    const ImVec2 list_size = ImVec2(0, 5 * ImGui::GetTextLineHeightWithSpacing());
    if (ImGui::BeginListBox("##Component Lists", list_size))
    {
        SE_SCOPE_DEFER{ ImGui::EndListBox(); };

        for (const TypeInfo& component_info : components)
        {
            ecs::IStorage* storage = world->GetStorage(component_info.type_id);
            if (!(storage && storage->Contains(entity)))
            {
                continue;
            }

            const String label = component_info.name;
            ImGui::PushID(label.CStr());

            // X 버튼으로 컴포넌트 삭제
            if (ImGui::SmallButton("X"))
            {
                component_to_remove = component_info.type_id;
            }
            ImGui::SameLine();

            if (ImGui::Selectable(label.CStr(), selected_component_id == component_info.type_id))
            {
                selected_component_id = component_info.type_id;
            }

            ImGui::PopID();
        }
    }

    // 컴포넌트 삭제 처리 (순회 완료 후)
    if (component_to_remove.IsValid())
    {
        if (const Optional ops = ecs::ComponentRegistry::Get().GetOps(component_to_remove))
        {
            ops->remove_component(*world, entity);
        }
        if (selected_component_id == component_to_remove)
        {
            selected_component_id = TypeId{};
        }
    }

    if (ImGui::BeginCombo("Add Component", "Select Component"))
    {
        SE_SCOPE_DEFER{ ImGui::EndCombo(); };

        for (const TypeInfo& component_info : components)
        {
            if (ecs::IStorage* storage = world->GetStorage(component_info.type_id))
            {
                const String add_label = component_info.name;
                if (ImGui::Selectable(add_label.CStr(), false))
                {
                    if (!storage->Contains(entity))
                    {
                        storage->EmplaceDefault(entity);
                    }
                }
            }
        }
    }

    ImGui::Separator();

    // 선택된 Component의 프로퍼티 렌더링
    if (!selected_component_id.IsValid())
    {
        return;
    }

    const Optional type_info_opt = TypeRegistry::Get().Find(selected_component_id);
    if (!type_info_opt)
    {
        return;
    }

    const Optional interface_opt = ecs::ComponentRegistry::Get().GetOps(selected_component_id);
    if (!interface_opt)
    {
        return;
    }

    void* component_data = interface_opt->get_component_mutable(*world, entity);
    if (!component_data)
    {
        return;
    }

    const StringView& type_name = type_info_opt->name;
    ImGui::Text("[%.*s]", static_cast<int>(type_name.ByteLen()), type_name.Data());
    ImGui::Separator();

    // TODO: 나중에 PropertyDrawer에서 컴포넌트별 커스텀 DrawProperties를 지원하도록 수정
    // TransformComponent는 Quaternion 대신 Euler 각도로 직관적으로 표시
    if (type_info_opt->type_id == TypeId::Get<TransformComponent>())
    {
        TransformComponent* tc = static_cast<TransformComponent*>(component_data);

        // Position
        ImGui::DragScalarN(
            "Position", ImGuiDataType_Double, &tc->position.x, 3,
            0.1f, nullptr, nullptr, nullptr
        );

        // Quaternion -> Euler (degrees) 변환 후 표시/편집 | TODO: cache해두고 값 변경시 Rot -> Quat로만 사용
        Rotator rot = tc->rotation.ToRotator();
        if (ImGui::DragScalarN(
            "Rotation", ImGuiDataType_Double, &rot.pitch.value, 3,
            0.1f, nullptr, nullptr, nullptr
        ))
        {
            tc->rotation = rot.ToQuaternion();
        }

        // Scale
        ImGui::DragScalarN(
            "Scale", ImGuiDataType_Double, &tc->scale.x, 3,
            0.1f, nullptr, nullptr, nullptr
        );
    }
    else
    {
        DrawerRegistry::Get().DrawProperties(*type_info_opt, component_data);
    }
}
} // namespace se::editor
