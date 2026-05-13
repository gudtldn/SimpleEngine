#include "UI/Panels/ViewportPanel.h"
#include "UI/ImGui/ImGuiWrapper.h"

#include "SimpleEditor/UI/EditorViewportSubsystem.h"
#include "SimpleEditor/UI/ImGuiHBox.h"

#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


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
            const u32 width = static_cast<u32>(viewport_size.x);
            const u32 height = static_cast<u32>(viewport_size.y);

            viewport_sys->UpdateViewportSize(viewport_id, width, height);

            // 뷰포트 로컬 커서 좌표 계산 및 전달
            const ImVec2 mouse_screen = ImGui::GetMousePos();
            viewport_sys->SetViewportCursorPosition(viewport_id, {
                mouse_screen.x - content_min.x,
                mouse_screen.y - content_min.y,
            });

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

    constexpr f32 TOOLBAR_HEIGHT = 28.0f;

    // 뷰포트가 너무 작으면 툴바 그리기 생략
    if (content_size.x < 100.0f || content_size.y < TOOLBAR_HEIGHT * 2)
    {
        return;
    }

    // 툴바 위젯 스타일 (HBox 스코프 밖에서 push/pop)
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.35f, 0.35f, 0.35f, 0.7f));

    {
        ImGuiHBox hbox = {
            viewport_id.CStr(),
            content_min,
            content_size,
            {
                .height = TOOLBAR_HEIGHT,
                .padding = 4.0f,
                .draw_background = true,
                .background_color = IM_COL32(30, 30, 30, 200),
                .draw_bottom_line = true,
                .bottom_line_color = IM_COL32(0, 0, 0, 128),
            }
        };

        // === 왼쪽: 기즈모 모드 버튼 ===
        const EGizmoMode cur_gizmo = viewport_sys->GetViewportGizmoMode(viewport_id);
        const f32 sq = hbox.ButtonHeight();

        bool gizmo_t = false, gizmo_r = false, gizmo_s = false; // NOLINT(*-isolate-declaration)
        hbox
            .ToggleButton("T", cur_gizmo == EGizmoMode::Translate, &gizmo_t, sq, "Translate")
            .ToggleButton("R", cur_gizmo == EGizmoMode::Rotate, &gizmo_r, sq, "Rotate")
            .ToggleButton("S", cur_gizmo == EGizmoMode::Scale, &gizmo_s, sq, "Scale");

        if (gizmo_t) { viewport_sys->SetViewportGizmoMode(viewport_id, EGizmoMode::Translate); }
        if (gizmo_r) { viewport_sys->SetViewportGizmoMode(viewport_id, EGizmoMode::Rotate); }
        if (gizmo_s) { viewport_sys->SetViewportGizmoMode(viewport_id, EGizmoMode::Scale); }

        // 좌표계 토글 (Scale 모드는 항상 Local)
        hbox.Separator();
        const bool force_local = cur_gizmo == EGizmoMode::Scale;
        const ECoordinateSpace current_coord = force_local
            ? ECoordinateSpace::Local
            : viewport_sys->GetViewportCoordinateSpace(viewport_id);
        bool coord_clicked = false;

        if (force_local) { ImGui::BeginDisabled(); }
        hbox.Button((current_coord == ECoordinateSpace::World) ? "World" : "Local", &coord_clicked);
        if (force_local) { ImGui::EndDisabled(); }

        if (coord_clicked && !force_local)
        {
            const ECoordinateSpace next = (current_coord == ECoordinateSpace::World)
                ? ECoordinateSpace::Local
                : ECoordinateSpace::World;
            viewport_sys->SetViewportCoordinateSpace(viewport_id, next);
        }

        // === 오른쪽: 뷰 모드 | 카메라 속도 | 렌더링 모드 | Show ===
        if (const Optional<EditorCameraState&> camera = viewport_sys->GetViewportCamera(viewport_id))
        {
            const EViewMode cur_view = viewport_sys->GetViewportViewMode(viewport_id);
            auto view_mode_label_fn = [](EViewMode mode) -> const char*
            {
                switch (mode)
                {
                case EViewMode::Perspective: return "Perspective";
                case EViewMode::Top:         return "Top";
                case EViewMode::Bottom:      return "Bottom";
                case EViewMode::Front:       return "Front";
                case EViewMode::Back:        return "Back";
                case EViewMode::Right:       return "Right";
                case EViewMode::Left:        return "Left";
                default:                     return "???";
                }
            };
            const char* view_label = view_mode_label_fn(cur_view);

            auto rendering_mode_label_fn = [](ERenderingMode mode) -> const char*
            {
                switch (mode)
                {
                case ERenderingMode::Lit:         return "Lit";
                case ERenderingMode::Unlit:       return "Unlit";
                case ERenderingMode::Wireframe:   return "Wireframe";
                case ERenderingMode::Normal:      return "Normal";
                case ERenderingMode::WorldNormal: return "WorldNormal";
                default:                          return "";
                }
            };

            hbox.Spring()
                .PopupButton(view_label, "##ViewMode", [&]
                {
                    auto view_item = [&](const char* label, EViewMode mode)
                    {
                        if (ImGui::Selectable(label, cur_view == mode))
                        {
                            viewport_sys->SetViewportViewMode(viewport_id, mode);
                        }
                    };

                    ImGui::Spacing();
                    ImGui::SeparatorText("Persp");
                    ImGui::Spacing();

                    view_item("Perspective", EViewMode::Perspective);

                    ImGui::Spacing();
                    ImGui::SeparatorText("Ortho");
                    ImGui::Spacing();

                    view_item("Top", EViewMode::Top);
                    view_item("Bottom", EViewMode::Bottom);
                    view_item("Front", EViewMode::Front);
                    view_item("Back", EViewMode::Back);
                    view_item("Right", EViewMode::Right);
                    view_item("Left", EViewMode::Left);
                })
                .Separator();

            if (viewport_sys->GetViewportViewMode(viewport_id) == EViewMode::Perspective)
            {
                hbox
                    .Label("Cam")
                    .Custom([&]
                    {
                        constexpr f64 MIN_SPEED = 0.01, MAX_SPEED = 1000.0; // NOLINT(*-isolate-declaration)
                        ImGui::DragScalarNInfinity(
                            "##CamSpeed", ImGuiDataType_Double, &camera->move_speed, 1,
                            0.1f, &MIN_SPEED, &MAX_SPEED, "%.1f"
                        );
                    }, 60.0f)
                    .Separator();
            }

            hbox
                .PopupButton(rendering_mode_label_fn(rendering_mode), "##RenderMode", [&]
                {
                    auto item = [&](const char* label, ERenderingMode mode)
                    {
                        if (ImGui::Selectable(label, rendering_mode == mode))
                        {
                            rendering_mode = mode;
                            viewport_sys->SetViewportRenderingMode(viewport_id, rendering_mode);
                        }
                    };

                    item("Lit", ERenderingMode::Lit);
                    item("Unlit", ERenderingMode::Unlit);
                    item("Wireframe", ERenderingMode::Wireframe);
                    item("Normal", ERenderingMode::Normal);
                    item("WorldNormal", ERenderingMode::WorldNormal);
                })
                .Separator()
                .PopupButton("Show", "##ShowFlags", [&]
                {
                    auto flag_checkbox = [&](const char* label, EShowFlag flag)
                    {
                        bool checked = show_flags.IsSet(flag);
                        if (ImGui::Checkbox(label, &checked))
                        {
                            show_flags.Toggle(flag);
                            viewport_sys->SetViewportShowFlags(viewport_id, show_flags);
                        }
                    };

                    flag_checkbox("Grid", EShowFlag::Grid);
                    flag_checkbox("AABB", EShowFlag::AABB);
                    flag_checkbox("StaticMesh", EShowFlag::StaticMesh);
                });
        }
    } // ~ImGuiHBox: PopID + 오른쪽 너비 캐시 저장

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(2);
}
} // namespace se::editor
