#include "UI/Panels/ViewportPanel.h"
#include "SimpleEditor/UI/EditorViewportSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"
#include "imgui_internal.h"


namespace se::editor
{
ViewportPanel::ViewportPanel(const StringName& in_viewport_id, bool default_visibility)
    : viewport_id(in_viewport_id)
{
    is_visible = default_visibility;
}

const char* ViewportPanel::GetName() const
{
    return viewport_id.CStr();
}

void ViewportPanel::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(GetName(), &is_visible);
    is_focused = ImGui::IsWindowFocused();
    is_hovered = ImGui::IsWindowHovered();
    {
        if (EditorViewportSubsystem* viewport_sys = GetSubsystem<EditorViewportSubsystem>())
        {
            viewport_sys->UpdateViewportFocus(viewport_id, ImGui::IsWindowFocused(), ImGui::IsWindowHovered());

            const ImVec2 content_min = ImGui::GetCursorScreenPos();
            const ImVec2 viewport_size = ImGui::GetContentRegionAvail();
            const uint32 width = static_cast<uint32>(viewport_size.x);
            const uint32 height = static_cast<uint32>(viewport_size.y);

            // 화면 크기 업데이트
            viewport_sys->UpdateViewportSize(viewport_id, width, height);

            if (void* texture_to_draw = viewport_sys->GetViewportTextureID(viewport_id))
            {
                ImGui::Image(texture_to_draw, viewport_size);
            }

            // 뷰포트 이미지 위에 오버레이 툴바를 렌더링합니다
            DrawToolbar(content_min, viewport_size);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::DrawToolbar(const ImVec2& content_min, const ImVec2& content_size)
{
    constexpr float TOOLBAR_HEIGHT = 28.0f;
    constexpr float PADDING = 4.0f;

    // 뷰포트가 너무 작으면 툴바 그리기 생략
    if (content_size.x < 100.0f || content_size.y < TOOLBAR_HEIGHT * 2)
    {
        return;
    }

    const ImVec2 toolbar_max = { content_min.x + content_size.x, content_min.y + TOOLBAR_HEIGHT };
    const float button_h = TOOLBAR_HEIGHT - (PADDING * 2);
    const ImVec2 icon_size = { button_h, button_h };

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 반투명 배경 + 하단 구분선
    draw_list->AddRectFilled(content_min, toolbar_max, IM_COL32(30, 30, 30, 200));
    draw_list->AddLine(
        { content_min.x, toolbar_max.y },
        { toolbar_max.x, toolbar_max.y },
        IM_COL32(0, 0, 0, 128)
    );

    // 툴바 위젯 스타일 설정
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.35f, 0.35f, 0.35f, 0.7f));

    // 뷰포트별 고유 ID (Main/Sub 뷰포트 위젯 ID 충돌 방지)
    ImGui::PushID(viewport_id.CStr());

    // 세로 구분선 헬퍼
    auto vertical_separator = []
    {
        ImGui::SameLine(0, 6.0f);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 6.0f);
    };

    // === 왼쪽 영역: 기즈모 모드 + 좌표계 ===
    ImGui::SetCursorScreenPos({ content_min.x + PADDING, content_min.y + PADDING });

    auto gizmo_button = [&](const char* label, const char* tooltip, EGizmoMode mode)
    {
        const bool is_active = (gizmo_mode == mode);
        if (is_active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.65f, 0.7f));
        }
        if (ImGui::Button(label, icon_size))
        {
            gizmo_mode = mode;
        }

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(tooltip);
            ImGui::EndTooltip();
        }

        if (is_active)
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    };

    gizmo_button("T", "Translate", EGizmoMode::Translate);
    gizmo_button("R", "Rotate", EGizmoMode::Rotate);
    gizmo_button("S", "Scale", EGizmoMode::Scale);

    vertical_separator();

    // 좌표계 토글
    const char* coord_label = (coordinate_space == ECoordinateSpace::World) ? "World" : "Local";
    if (ImGui::Button(coord_label, { 0, button_h }))
    {
        coordinate_space = (coordinate_space == ECoordinateSpace::World)
            ? ECoordinateSpace::Local
            : ECoordinateSpace::World;
    }

    // === 오른쪽 영역: 카메라 속도 ===
    if (EditorViewportSubsystem* viewport_sys = GetSubsystem<EditorViewportSubsystem>())
    {
        if (const Optional<EditorCameraState&> camera = viewport_sys->GetViewportCamera(viewport_id))
        {
            constexpr float SPEED_WIDGET_WIDTH = 60.0f;
            const ImVec2 cam_label_size = ImGui::CalcTextSize("Cam");
            constexpr float LABEL_SPACING = 4.0f;
            const float right_width = cam_label_size.x + LABEL_SPACING + SPEED_WIDGET_WIDTH;

            ImGui::SetCursorScreenPos({
                toolbar_max.x - right_width - PADDING,
                content_min.y + PADDING
            });

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cam");
            ImGui::SameLine(0, LABEL_SPACING);

            ImGui::PushItemWidth(SPEED_WIDGET_WIDTH);
            float speed = static_cast<float>(camera->move_speed);
            if (ImGui::DragFloat("##CamSpeed", &speed, 0.1f, 0.1f, 1000.0f, "%.1f"))
            {
                camera->move_speed = static_cast<double>(speed);
            }
            ImGui::PopItemWidth();
        }
    }

    ImGui::PopID();
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(2);
}
} // namespace se::editor
