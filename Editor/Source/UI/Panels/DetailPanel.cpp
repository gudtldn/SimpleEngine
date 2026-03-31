#include "UI/Panels/DetailPanel.h"
#include "UI/ImGui/ImGuiWrapper.h"

#include "SimpleEditor/Core/EditorSubsystem.h"
#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ComponentRegistry.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"

#include <ranges>


namespace se::editor
{
using namespace se::math;

const char* DetailPanel::GetName() const
{
    return "Detail";
}

void DetailPanel::DrawContent()
{
    const auto [entity_subsystem, editor_subsystem] = GetSubsystems<EntitySubsystem, EditorSubsystem>();
    if (!(entity_subsystem && editor_subsystem))
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

    const Entity entity = selection.GetPrimarySelectedEntity().Value();
    World* world = &entity_subsystem->GetMainWorld().GetWorld();

    // 선택된 Entity가 바뀌면 Rotation 캐시 초기화
    if (last_selected_entity != entity)
    {
        rotator_cache.Remove(last_selected_entity);
        last_selected_entity = entity;
    }

    ImGui::Separator();

    if (ImGui::Button("Add Component", ImVec2(-1, 0)))
    {
        ImGui::OpenPopup("ComponentSearchPopup");
    }

    // 버튼 바로 아래에 팝업을 띄움
    ImVec2 popup_pos = ImGui::GetItemRectMin(); // 버튼의 좌상단 좌표
    popup_pos.y += ImGui::GetItemRectSize().y;  // y좌표에 버튼 높이를 더함
    ImGui::SetNextWindowPos(popup_pos);

    if (ImGui::BeginPopup("ComponentSearchPopup"))
    {
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }

        static char filter[64] = "";
        ImGui::InputTextWithHint("##Filter", "Search...", filter, std::size(filter));
        ImGui::Separator();

        usize found_count = 0;
        for (const TypeId& type_id : ComponentRegistry::Get().GetOperators() | std::views::keys)
        {
            IStorage* storage = world->GetStorage(type_id);
            if (!storage)
            {
                continue;
            }

            const Optional type_info_opt = TypeRegistry::Get().Find(type_id);
            if (!type_info_opt)
            {
                continue;
            }

            const String add_label = type_info_opt->name;
            // TODO: Case-Insensitive로 비교 하도록 수정
            // ReSharper disable once CppRedundantCastExpression
            if (add_label.Contains(static_cast<const char*>(filter))) // 리터럴 생성자 오버로딩을 사용하지 않도록 함
            {
                ++found_count;
                if (ImGui::Selectable(add_label.CStr(), false))
                {
                    if (!storage->Contains(entity))
                    {
                        storage->EmplaceDefault(entity);
                    }

                    filter[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        if (found_count == 0)
        {
            ImGui::TextDisabled("No components found matching filter: \"%s\"", filter);
        }

        ImGui::EndPopup();
    }

    TypeId component_to_remove;
    for (const auto& [component_type, component_ops] : ComponentRegistry::Get().GetOperators())
    {
        const IStorage* storage = world->GetStorage(component_type);

        // Entity가 가지고 있지 않은 Component는 건너뜀
        if (!(storage && storage->Contains(entity)))
        {
            continue;
        }

        // Component의 타입 정보
        const TypeInfo& type_info = TypeRegistry::Get().FindChecked(component_type);

        const String label = type_info.name;
        ImGui::PushID(label.CStr());
        SE_SCOPE_DEFER{ ImGui::PopID(); };

        const bool header_is_open = ImGui::CollapsingHeader(label.CStr(), ImGuiTreeNodeFlags_DefaultOpen);

        if (ImGui::BeginPopupContextItem("ComponentContextMenu"))
        {
            if (ImGui::MenuItem("Remove Component"))
            {
                component_to_remove = component_type;
            }
            ImGui::EndPopup();
        }

        if (header_is_open)
        {
            void* component_data = component_ops.get_component_mutable(*world, entity);
            if (!component_data)
            {
                continue;
            }

            // TODO: 나중에 PropertyDrawer에서 컴포넌트별 커스텀 DrawProperties를 지원하도록 수정
            // TransformComponent는 Quaternion 대신 Euler 각도로 직관적으로 표시
            if (component_type == TypeId::Get<TransformComponent>())
            {
                TransformComponent* transform_component = static_cast<TransformComponent*>(component_data);

                // Position
                ImGui::DragScalarNInfinity(
                    "Position", ImGuiDataType_Double, &transform_component->position.x, 3,
                    0.1f, nullptr, nullptr, nullptr
                );

                // 편집 중이 아닐 때만 외부 변경을 감지해 Quat -> Euler 재계산
                CachedRotator& cached = rotator_cache[entity];
                if (!cached.is_editing && !cached.source_quat.IsNearlyEqual(transform_component->rotation))
                {
                    cached.euler = transform_component->rotation.ToRotator();
                    cached.source_quat = transform_component->rotation;
                }

                if (ImGui::DragScalarNInfinity(
                    "Rotation", ImGuiDataType_Double, &cached.euler.pitch.value, 3,
                    0.1f, nullptr, nullptr, nullptr
                ))
                {
                    transform_component->rotation = cached.euler.ToQuaternion();
                    cached.source_quat = transform_component->rotation;
                }
                cached.is_editing = ImGui::IsItemActive();

                // Scale
                ImGui::DragScalarNInfinity(
                    "Scale", ImGuiDataType_Double, &transform_component->scale.x, 3,
                    0.1f, nullptr, nullptr, nullptr
                );
            }
            else
            {
                DrawerRegistry::Get().DrawProperties(type_info, component_data);
            }
        }
    }

    // 컴포넌트 삭제 처리 (순회 완료 후)
    if (component_to_remove.IsValid())
    {
        if (const Optional ops = ComponentRegistry::Get().GetOps(component_to_remove))
        {
            ops->remove_component(*world, entity);
        }
    }
}
} // namespace se::editor
