#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "imgui.h"
#include "SimpleEngine/Core/Container/Optional.h"


namespace se::editor
{
/** ImGuiHBox의 설정 */
struct ImGuiHBoxConfig
{
    float height = 0.0f;
    float padding = 0.0f;

    bool draw_background = false;
    ImU32 background_color = IM_COL32(0, 0, 0, 0);

    bool draw_bottom_line = false;
    ImU32 bottom_line_color = IM_COL32(0, 0, 0, 0);
};

/**
 * ImGui 가로 레이아웃 컨테이너
 * Spring()을 기준으로 왼쪽/오른쪽 정렬을 선언적으로 표현합니다.
 *
 * 오른쪽 영역 너비는 ImGuiStorage에 1프레임 지연 캐시되어,
 * 두 번째 프레임부터 우측 정렬이 정확해집니다.
 *
 * usage:
 *   ImGuiHBox hbox("id", min_pos, size, { .height = 28.0f, .draw_background = true });
 *   hbox.ToggleButton("T", is_translate, &clicked)
 *       .Spring()
 *       .PopupButton("Perspective", "##ViewMode", [&]{ ... });
 */
class SE_EDITOR_API ImGuiHBox
{
public:
    ImGuiHBox(
        const char* in_id,
        Optional<ImVec2> min_pos_opt = NullOpt,
        Optional<ImVec2> size_opt = NullOpt,
        const ImGuiHBoxConfig& in_config = {}
    );
    ~ImGuiHBox();

    ImGuiHBox(const ImGuiHBox&) = delete;
    ImGuiHBox& operator=(const ImGuiHBox&) = delete;
    ImGuiHBox(ImGuiHBox&&) = delete;
    ImGuiHBox& operator=(ImGuiHBox&&) = delete;

public:
    /** 오른쪽 영역의 시작점을 표시합니다. Spring() 이후 아이템은 우측 정렬됩니다. */
    ImGuiHBox& Spring();

    /** 세로 구분선을 그립니다. */
    ImGuiHBox& Separator();

    /** 프레임 패딩에 맞춰 정렬된 텍스트를 표시합니다. */
    ImGuiHBox& Label(const char* text);

    /** 일반 버튼. out_clicked가 있으면 클릭 여부를 기록합니다. */
    ImGuiHBox& Button(
        const char* label,
        bool* out_clicked = nullptr,
        float width = 0.0f,
        const char* tooltip = nullptr
    );

    /** is_active가 true이면 강조 스타일로 표시되는 토글 버튼. */
    ImGuiHBox& ToggleButton(
        const char* label,
        bool is_active,
        bool* out_clicked = nullptr,
        float width = 0.0f,
        const char* tooltip = nullptr
    );

    /** DragScalar 위젯. width > 0이면 아이템 너비를 고정합니다. */
    ImGuiHBox& DragScalar(
        const char* label,
        ImGuiDataType type,
        void* data,
        float speed = 1.0f,
        const void* min_val = nullptr,
        const void* max_val = nullptr,
        const char* format = nullptr,
        float width = 0.0f
    );

    /**
     * 버튼을 클릭하면 팝업을 여는 복합 위젯.
     * popup_id: HBox 내에서 고유한 ID (예: "##ViewMode").
     * draw_popup_fn: 팝업 내용을 그리는 콜백.
     */
    template <typename Fn>
    ImGuiHBox& PopupButton(const char* label, const char* popup_id, Fn&& draw_popup_fn, float width = 0.0f)
    {
        EnsureSameLine();
        if (ImGui::Button(label, { width, button_h }))
        {
            ImGui::OpenPopup(popup_id);
        }
        if (ImGui::BeginPopup(popup_id))
        {
            draw_popup_fn();
            ImGui::EndPopup();
        }
        return *this;
    }

    /**
     * 임의의 ImGui 위젯을 인라인으로 삽입합니다.
     * item_width > 0이면 PushItemWidth/PopItemWidth를 감쌉니다.
     */
    template <typename Fn>
    ImGuiHBox& Custom(Fn&& fn, float item_width = 0.0f)
    {
        EnsureSameLine();
        if (item_width > 0.0f)
        {
            ImGui::PushItemWidth(item_width);
        }
        fn();
        if (item_width > 0.0f)
        {
            ImGui::PopItemWidth();
        }
        return *this;
    }

    /** 버튼 높이를 반환합니다. (height - padding * 2) */
    [[nodiscard]] float ButtonHeight() const { return button_h; }

private:
    void EnsureSameLine();

private:
    const char* id;
    ImVec2 min_pos;
    ImVec2 max_pos; // min_pos + (size.x, config.height)
    ImGuiHBoxConfig config;
    float button_h; // height - padding * 2

    bool spring_done = false;
    bool first_item = true;
    float spring_cursor_x = 0.0f; // Spring() 호출 시 설정된 커서 x

    ImGuiStorage* storage = nullptr;
    ImGuiID spring_width_key = 0;
};
} // namespace se::editor
