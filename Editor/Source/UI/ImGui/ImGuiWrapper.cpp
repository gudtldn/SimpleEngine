#include "ImGuiWrapper.h"

#include "imgui_internal.h"

#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace ImGui
{
bool ImGui::DragScalarNInfinity(
    const char* label, ImGuiDataType data_type, void* p_data, int components,
    float v_speed, const void* p_min, const void* p_max, const char* format,
    ImGuiSliderFlags flags
)
{
    se::InputSubsystem* input_subsystem = se::GetSubsystem<se::InputSubsystem>();

    static ImGuiID dragging_id = 0;
    static se::Vector2f start_mouse_pos = se::Vector2f::Zero();

    ImGuiIO& io = GetIO();

    // 드래그 중 ImGui가 ActiveID를 외부에서 클리어한 경우(예: 위젯이 스크롤로 뷰 밖으로 나감)를
    // 감지하여 Relative Mouse Mode가 꺼지지 않는 상태를 방지합니다.
    if (dragging_id != 0 && GetActiveID() != dragging_id && input_subsystem)
    {
        input_subsystem->SetRelativeMouseMode(false);
        dragging_id = 0;
    }

    // DragScalarN이 io.MouseDelta를 소비하므로 반드시 호출 전에 주입하고,
    // 호출 후 원본을 복구해 이후 렌더되는 다른 위젯에 조작된 델타가 전파되지 않도록 합니다.
    const ImVec2 original_delta = io.MouseDelta;

    if (dragging_id != 0 && input_subsystem)
    {
        const se::Vector2f mouse_delta = input_subsystem->GetMouseDelta();
        io.MouseDelta = { mouse_delta.x, mouse_delta.y };
    }

    const bool value_changed = DragScalarN(label, data_type, p_data, components, v_speed, p_min, p_max, format, flags);

    io.MouseDelta = original_delta;

    if (input_subsystem)
    {
        if (IsItemActivated())
        {
            start_mouse_pos = input_subsystem->GetLocalMousePosition();

            // 텍스트 직접 입력 모드(Ctrl+Click 등)가 아닐 때만 드래그 모드로 전환합니다.
            // TempInputIsActive가 true이면 ImGui가 InputText를 열어 직접 입력 중인 상태입니다.
            if (!TempInputIsActive(GetItemID()))
            {
                input_subsystem->SetRelativeMouseMode(true);
                dragging_id = GetItemID();
            }
        }

        if (IsItemDeactivated() && dragging_id == GetItemID())
        {
            input_subsystem->SetLocalMousePosition(start_mouse_pos);
            input_subsystem->SetRelativeMouseMode(false);
            dragging_id = 0;
        }
    }

    return value_changed;
}
} // namespace ImGui
