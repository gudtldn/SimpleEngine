#include "UI/Panels/ViewportPanel.h"
#include "UI/ImGui/ImGuiWrapper.h"

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

    // 우클릭 시 ImGui 포커스를 명시적으로 이 창으로 이동
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::SetWindowFocus();
    }

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

            viewport_sys->UpdateViewportSize(viewport_id, width, height);

            if (void* texture_to_draw = viewport_sys->GetViewportTextureID(viewport_id))
            {
                ImGui::Image(texture_to_draw, viewport_size);
            }

            // 뷰포트 이미지 위에 오버레이 툴바를 렌더링
            DrawToolbar(content_min, viewport_size);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::DrawToolbar(const ImVec2& content_min, const ImVec2& content_size)
{
    EditorViewportSubsystem* viewport_sys = GetSubsystem<EditorViewportSubsystem>();
    if (!viewport_sys)
    {
        return;
    }

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

    const EGizmoMode current_gizmo_mode = viewport_sys->GetViewportGizmoMode(viewport_id);

    auto gizmo_button = [&](const char* label, const char* tooltip, EGizmoMode mode)
    {
        const bool is_active = (current_gizmo_mode == mode);
        if (is_active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.65f, 0.7f));
        }
        if (ImGui::Button(label, icon_size))
        {
            viewport_sys->SetViewportGizmoMode(viewport_id, mode);
        }

        if (ImGui::IsItemHovered())
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
    const ECoordinateSpace current_coord_space = viewport_sys->GetViewportCoordinateSpace(viewport_id);
    const char* coord_label = (current_coord_space == ECoordinateSpace::World) ? "World" : "Local";
    if (ImGui::Button(coord_label, { 0, button_h }))
    {
        const ECoordinateSpace next = (current_coord_space == ECoordinateSpace::World)
            ? ECoordinateSpace::Local
            : ECoordinateSpace::World;
        viewport_sys->SetViewportCoordinateSpace(viewport_id, next);
    }

    // === 오른쪽 영역: 카메라 속도 | 렌더링 모드 | Show 플래그 ===
    if (const Optional<EditorCameraState&> camera = viewport_sys->GetViewportCamera(viewport_id))
    {
        constexpr float SPEED_WIDGET_WIDTH = 60.0f;
        constexpr float LABEL_SPACING = 4.0f;
        constexpr float SEP_WIDTH = 13.0f; // SameLine(6) + Separator(1) + SameLine(6)

        auto rendering_mode_label = [](graphics::ERenderingMode mode) -> const char*
        {
            switch (mode)
            {
            case graphics::ERenderingMode::Lit:       return "Lit";
            case graphics::ERenderingMode::Unlit:     return "Unlit";
            case graphics::ERenderingMode::Wireframe: return "Wireframe";
            default:                                  return "";
            }
        };

        const ImVec2 cam_label_size = ImGui::CalcTextSize("Cam");
        const float mode_btn_w = ImGui::CalcTextSize(rendering_mode_label(rendering_mode)).x + (ImGui::GetStyle().FramePadding.x * 2.0f);
        const float show_btn_w = ImGui::CalcTextSize("Show").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
        const float right_width = cam_label_size.x + LABEL_SPACING + SPEED_WIDGET_WIDTH + SEP_WIDTH + mode_btn_w + SEP_WIDTH + show_btn_w;

        ImGui::SetCursorScreenPos({
            toolbar_max.x - right_width - PADDING,
            content_min.y + PADDING
        });

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Cam");
        ImGui::SameLine(0, LABEL_SPACING);

        ImGui::PushItemWidth(SPEED_WIDGET_WIDTH);
        constexpr double MIN_SPEED = 0.01, MAX_SPEED = 1000.0; // NOLINT(*-isolate-declaration)
        ImGui::DragScalarNInfinity(
            "##CamSpeed", ImGuiDataType_Double, &camera->move_speed, 1,
            0.1f, &MIN_SPEED, &MAX_SPEED, "%.1f"
        );
        ImGui::PopItemWidth();

        vertical_separator();

        if (ImGui::Button(rendering_mode_label(rendering_mode), { 0, button_h }))
        {
            ImGui::OpenPopup("RenderingModePopup");
        }

        vertical_separator();

        if (ImGui::Button("Show", { 0, button_h }))
        {
            ImGui::OpenPopup("ShowFlagsPopup");
        }
    }

    if (ImGui::BeginPopup("RenderingModePopup"))
    {
        auto rendering_mode_item = [&](const char* label, graphics::ERenderingMode mode)
        {
            if (ImGui::Selectable(label, rendering_mode == mode))
            {
                rendering_mode = mode;
                viewport_sys->SetViewportRenderingMode(viewport_id, rendering_mode);
            }
        };

        rendering_mode_item("Lit", graphics::ERenderingMode::Lit);
        rendering_mode_item("Unlit", graphics::ERenderingMode::Unlit);
        rendering_mode_item("Wireframe", graphics::ERenderingMode::Wireframe);

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("ShowFlagsPopup"))
    {
        auto show_flag_checkbox = [&](const char* label, graphics::EShowFlag flag)
        {
            bool checked = show_flags.IsSet(flag);
            if (ImGui::Checkbox(label, &checked))
            {
                show_flags.Toggle(flag);
                viewport_sys->SetViewportShowFlags(viewport_id, show_flags);
            }
        };

        show_flag_checkbox("Grid", graphics::EShowFlag::Grid);
        show_flag_checkbox("AABB", graphics::EShowFlag::AABB);

        ImGui::EndPopup();
    }

    ImGui::PopID();
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(2);
}
} // namespace se::editor
