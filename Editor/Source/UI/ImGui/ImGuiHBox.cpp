#include "SimpleEditor/UI/ImGuiHBox.h"

#include "imgui.h"
#include "imgui_internal.h"


namespace se::editor
{
ImGuiHBox::ImGuiHBox(
    const char* in_id,
    Optional<ImVec2> min_pos_opt,
    Optional<ImVec2> size_opt,
    const ImGuiHBoxConfig& in_config
)
    : id(in_id)
    , config(in_config)
{
    // config에 값이 없으면 ImGui의 현재 글꼴 + 패딩 크기를 사용
    const float actual_height = config.height > 0.0f
                                    ? config.height
                                    : ImGui::GetFrameHeight();

    // 버튼 높이 계산
    button_h = actual_height - (config.padding * 2.0f);

    // 값을 안 넘기면 현재 ImGui 커서 위치를 시작점으로 사용
    min_pos = min_pos_opt.ValueOr(ImGui::GetCursorScreenPos());

    // 크기를 안 넘기면 현재 창의 남은 가로 너비를 전부 사용
    const ImVec2 size = size_opt.ValueOr(ImVec2{ ImGui::GetContentRegionAvail().x, in_config.height });
    max_pos = { min_pos.x + size.x, min_pos.y + actual_height };

    // 배경 및 하단 구분선 그리기
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (config.draw_background)
    {
        draw_list->AddRectFilled(min_pos, max_pos, config.background_color);
    }
    if (config.draw_bottom_line)
    {
        draw_list->AddLine(
            { min_pos.x, max_pos.y },
            { max_pos.x, max_pos.y },
            config.bottom_line_color
        );
    }

    // PushID -> GetID -> GetStateStorage 순서로 호출
    // (key가 viewport별로 고유하도록 ID 스택에 id를 먼저 삽입)
    ImGui::PushID(id);
    spring_width_key = ImGui::GetID("##spring_w");
    storage = ImGui::GetStateStorage();

    // 커서를 왼쪽 상단 패딩 위치로 이동
    ImGui::SetCursorScreenPos({ min_pos.x + config.padding, min_pos.y + config.padding });
}

ImGuiHBox::~ImGuiHBox()
{
    // Spring() 이후 오른쪽 영역 너비를 측정해 다음 프레임에 사용
    if (spring_done && storage != nullptr)
    {
        // GetItemRectMax().x: 마지막으로 그린 아이템의 오른쪽 끝 x
        const float right_w = ImGui::GetItemRectMax().x - spring_cursor_x;
        storage->SetFloat(spring_width_key, right_w > 0.0f ? right_w : 0.0f);
    }

    ImGui::PopID();
}

ImGuiHBox& ImGuiHBox::Spring()
{
    spring_done = true;

    // 캐시된 오른쪽 영역 너비로 시작 x를 계산
    const float cached_right_w = storage->GetFloat(spring_width_key, 0.0f);
    const float right_start_x = (cached_right_w > 0.0f)
                                    ? (max_pos.x - cached_right_w - config.padding)
                                    : (max_pos.x - config.padding); // 첫 프레임: 오른쪽 끝에서 시작

    spring_cursor_x = right_start_x;
    first_item = true; // Spring 직후 EnsureSameLine 호출 억제

    ImGui::SetCursorScreenPos({ right_start_x, min_pos.y + config.padding });
    return *this;
}

ImGuiHBox& ImGuiHBox::Separator()
{
    if (!first_item)
    {
        ImGui::SameLine(0, 6.0f);
    }
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine(0, 6.0f);

    // 두 번째 SameLine으로 이미 같은 줄에 있으므로 다음 아이템의 SameLine 방지
    first_item = true;
    return *this;
}

ImGuiHBox& ImGuiHBox::Label(const char* text)
{
    EnsureSameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text);
    return *this;
}

ImGuiHBox& ImGuiHBox::Button(const char* label, bool* out_clicked, float width, const char* tooltip)
{
    EnsureSameLine();
    const bool clicked = ImGui::Button(label, { width, button_h });
    if (out_clicked)
    {
        *out_clicked = clicked;
    }
    if (tooltip && ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        ImGui::EndTooltip();
    }
    return *this;
}

ImGuiHBox& ImGuiHBox::ToggleButton(const char* label, bool is_active, bool* out_clicked, float width, const char* tooltip)
{
    EnsureSameLine();
    if (is_active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.65f, 0.7f));
    }
    const bool clicked = ImGui::Button(label, { width, button_h });
    if (is_active)
    {
        ImGui::PopStyleColor();
    }
    if (out_clicked)
    {
        *out_clicked = clicked;
    }
    if (tooltip && ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        ImGui::EndTooltip();
    }
    return *this;
}

ImGuiHBox& ImGuiHBox::DragScalar(
    const char* label,
    ImGuiDataType type,
    void* data,
    float speed,
    const void* min_val,
    const void* max_val,
    const char* format,
    float width
)
{
    EnsureSameLine();
    if (width > 0.0f)
    {
        ImGui::PushItemWidth(width);
    }
    ImGui::DragScalar(label, type, data, speed, min_val, max_val, format);
    if (width > 0.0f)
    {
        ImGui::PopItemWidth();
    }
    return *this;
}

void ImGuiHBox::EnsureSameLine()
{
    if (first_item)
    {
        first_item = false;
    }
    else
    {
        ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
    }
}
} // namespace se::editor
