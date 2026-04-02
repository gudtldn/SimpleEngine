#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Math/TransformUtility.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Debug/DebugDraw.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"

#include <ranges>


namespace se::editor
{
using namespace se::math;

SE_REGISTER_SUBSYSTEM(EditorViewportSubsystem)
    .DependsOn<RenderSubsystem>()
    .DependsOn<InputSubsystem>();

SE_BEGIN_REFLECT(EditorViewportSubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(EditorViewportSubsystem)

bool EditorViewportSubsystem::Initialize()
{
    input_subsystem = &GetSubsystemChecked<InputSubsystem>();
    render_device = &GetSubsystemChecked<RenderSubsystem>().GetRenderDevice();
    return true;
}

void EditorViewportSubsystem::Release()
{
    if (render_device)
    {
        for (const ViewportState& state : viewports | std::views::values)
        {
            if (state.color_texture)
            {
                render_device->DestroyTexture(state.color_texture);
            }
        }
    }
    viewports.Clear();
    render_device = nullptr;
    input_subsystem = nullptr;
}

void EditorViewportSubsystem::Update(float delta_time)
{
    // 우클릭으로 호버된 뷰포트의 카메라 제어를 활성화
    if (input_subsystem->IsMouseButtonPressed(EMouseButton::Right))
    {
        for (const auto& [id, state] : viewports)
        {
            if (state.is_hovered)
            {
                active_camera_viewport = id;
                last_mouse_pos = input_subsystem->GetLocalMousePosition();

                input_subsystem->SetRelativeMouseMode(true);
                break;
            }
        }
    }
    else if (input_subsystem->IsMouseButtonReleased(EMouseButton::Right))
    {
        if (active_camera_viewport != StringName::None)
        {
            ViewportState& release_state = viewports[active_camera_viewport];
            EditorCameraState& release_cam = release_state.GetActiveCamera();
            release_cam.velocity = Vector3::Zero();

            input_subsystem->SetLocalMousePosition(last_mouse_pos);
            input_subsystem->SetRelativeMouseMode(false);

            active_camera_viewport = StringName::None;
            last_mouse_pos = Vector2f::Zero();
        }
    }

    // TODO: 카메라 모드(flythrough/orbit 등)가 추가되거나 Update()가 비대해지면 EditorCameraSubsystem으로 분리할 것
    // 활성 카메라 입력 처리
    if (active_camera_viewport != StringName::None && input_subsystem->IsMouseButtonDown(EMouseButton::Right))
    {
        ViewportState& active_state = viewports[active_camera_viewport];
        EditorCameraState& camera = active_state.GetActiveCamera();

        const Vector2f mouse_delta = input_subsystem->GetMouseDelta();

        if (active_state.view_mode == EViewMode::Perspective)
        {
            // Perspective Flythrough
            // 마우스 회전 yaw(Z축), pitch(X축) | TODO: 나중에 쿼터니언으로 수정
            camera.rotation.yaw -= Degree{ mouse_delta.x * camera.look_sensitivity };
            camera.rotation.yaw = Degree{ Fmod(*camera.rotation.yaw, 360.0) };
            camera.rotation.pitch -= Degree{ mouse_delta.y * camera.look_sensitivity };
            camera.rotation.pitch = Degree{ Clamp(*camera.rotation.pitch, -89.0, 89.0) };

            // WASD/QE 이동
            const Vector3 forward = camera.rotation.GetForwardVector();
            const Vector3 right = camera.rotation.GetRightVector();

            Vector3 target_velocity = Vector3::Zero();
            if (input_subsystem->IsKeyDown(EKeyCode::W)) { target_velocity += forward; }
            if (input_subsystem->IsKeyDown(EKeyCode::S)) { target_velocity -= forward; }
            if (input_subsystem->IsKeyDown(EKeyCode::D)) { target_velocity += right; }
            if (input_subsystem->IsKeyDown(EKeyCode::A)) { target_velocity -= right; }
            if (input_subsystem->IsKeyDown(EKeyCode::E)) { target_velocity += Vector3::Up(); }
            if (input_subsystem->IsKeyDown(EKeyCode::Q)) { target_velocity -= Vector3::Up(); }

            if (!target_velocity.IsNearlyZero())
            {
                target_velocity = target_velocity.GetNormalized() * camera.move_speed;
            }

            // Exponential smoothing: frame-rate independent 가속/감속 (smoothing이 클수록 반응이 빠름)
            constexpr double SMOOTHING = 12.0;
            const double alpha = 1.0 - Exp(-SMOOTHING * static_cast<double>(delta_time));
            camera.velocity = camera.velocity + (target_velocity - camera.velocity) * alpha;

            camera.position += camera.velocity * delta_time;

            // Perspective에서 스크롤로 이동 속도 조절
            const float scroll = input_subsystem->GetMouseWheel().y;
            if (scroll != 0.0f)
            {
                camera.move_speed *= (scroll > 0.0f) ? 1.1 : (1.0 / 1.1);
                camera.move_speed = Clamp(camera.move_speed, 0.1, 1000.0);
            }
        }
        else
        {
            // Pan 평면의 로컬 기저 벡터 도출 (Top/Bottom 뷰 특이점 우회)
            const Vector3 forward = camera.rotation.GetForwardVector();
            const bool is_top_bottom = (active_state.view_mode == EViewMode::Top || active_state.view_mode == EViewMode::Bottom);

            const Vector3 view_up = is_top_bottom ? Vector3{ 0.0, 1.0, 0.0 } : Vector3::Up();
            const Vector3 screen_right = view_up.Cross(forward).GetNormalized();
            const Vector3 screen_up = forward.Cross(screen_right);

            // 마우스 픽셀 이동량을 월드 스케일로 환산 (줌 레벨에 비례)
            const double viewport_w = static_cast<double>(active_state.render_view.width);
            const double pan_scale = camera.ortho_width / viewport_w;

            // 카메라 이동
            camera.position += screen_right * (static_cast<double>(mouse_delta.x) * pan_scale);
            camera.position += screen_up * (static_cast<double>(mouse_delta.y) * pan_scale);
        }

        // 카메라가 활성화 되어있는 동안, 마우스를 last_pos에 고정
        input_subsystem->SetLocalMousePosition(last_mouse_pos);
    }

    // Ortho 줌: 우클릭 없이 호버 상태에서 스크롤로 가능
    {
        const float ortho_scroll = input_subsystem->GetMouseWheel().y;
        if (ortho_scroll != 0.0f)
        {
            for (ViewportState& state : viewports | std::views::values)
            {
                if (state.is_hovered && state.IsOrthographicView())
                {
                    state.ortho_camera.ortho_width *= (ortho_scroll > 0.0f) ? (1.0 / 1.1) : 1.1;
                    state.ortho_camera.ortho_width = Clamp(state.ortho_camera.ortho_width, 0.1, 10000.0);
                    break;
                }
            }
        }
    }

    // 월드 기준 좌표축을 매 프레임 그리기
    DrawDebugWorldAxes();

    // 모든 뷰포트의 render_view에 최신 카메라 행렬을 반영
    for (ViewportState& state : viewports | std::views::values)
    {
        if (state.render_view.width == 0 || state.render_view.height == 0)
        {
            continue;
        }

        const EditorCameraState& camera = state.GetActiveCamera();
        const Vector3 forward_dir = camera.rotation.GetForwardVector();
        const double aspect = static_cast<double>(state.render_view.width) / static_cast<double>(state.render_view.height);

        // Top/Bottom 뷰는 Up을 Y축을 기준으로 설정
        const bool is_top_bottom = (state.view_mode == EViewMode::Top || state.view_mode == EViewMode::Bottom);
        const Vector3 view_up = is_top_bottom ? Vector3{ 0.0, 1.0, 0.0 } : Vector3::Up();

        if (state.IsPerspectiveView())
        {
            state.render_view.projection_matrix = TransformUtility::MakePerspectiveMatrix(
                Radian{ camera.fov_y },
                aspect,
                camera.near_plane,
                camera.far_plane
            );
            state.render_view.view_matrix = TransformUtility::MakeViewMatrix(
                camera.position,
                camera.position + forward_dir,
                view_up
            );
        }
        else
        {
            const double ortho_height = camera.ortho_width / aspect;
            state.render_view.projection_matrix = TransformUtility::MakeOrthographicMatrix(
                camera.ortho_width,
                ortho_height,
                camera.near_plane,
                camera.far_plane
            );

            const double depth_half = camera.far_plane * 0.5;
            const Vector3 ortho_eye = camera.position
                - forward_dir * forward_dir.Dot(camera.position) // Depth 성분 제거
                - forward_dir * depth_half;                      // 이후 클리핑 공간의 정중앙에 물체를 두기 위해 카메라를 뒤로 이동
            state.render_view.view_matrix = TransformUtility::MakeViewMatrix(
                ortho_eye,
                ortho_eye + forward_dir,
                view_up
            );
        }
        state.render_view.near_plane = static_cast<float>(camera.near_plane);
        state.render_view.far_plane = static_cast<float>(camera.far_plane);
    }
}

void EditorViewportSubsystem::UpdateViewportSize(const StringName& viewport_id, uint32 new_width, uint32 new_height)
{
    if (new_width == 0 || new_height == 0)
    {
        if (const auto state = viewports.Find(viewport_id))
        {
            if (state->color_texture)
            {
                render_device->DestroyTexture(
                    std::exchange(state->color_texture, {})
                );
            }
        }
        return;
    }

    // 텍스처가 없거나, 크기가 변경된 경우 재생성
    ViewportState& state = viewports[viewport_id];
    if (
        !state.color_texture.IsValid()
        || state.render_view.width != new_width
        || state.render_view.height != new_height
    )
    {
        if (state.color_texture)
        {
            render_device->DestroyTexture(state.color_texture);
        }

        state.color_texture = render_device->CreateTexture({
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = new_width,
            .height = new_height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        });
        state.render_view.width = new_width;
        state.render_view.height = new_height;
        state.color_target_name = viewport_id;
        state.depth_target_name = String::Format("{}_Depth", viewport_id);
    }
}

void EditorViewportSubsystem::UpdateViewportFocus(const StringName& viewport_id, bool focused, bool hovered)
{
    if (const auto state = viewports.Find(viewport_id))
    {
        state->is_focused = focused;
        state->is_hovered = hovered;
    }

    if (focused)
    {
        focused_viewport = viewport_id;
    }
    else if (focused_viewport == viewport_id)
    {
        focused_viewport = StringName::None;
    }
}

void EditorViewportSubsystem::SetViewportRenderingMode(const StringName& viewport_id, graphics::ERenderingMode mode)
{
    if (const auto state = viewports.Find(viewport_id))
    {
        state->render_view.rendering_mode = mode;
    }
}

void EditorViewportSubsystem::SetViewportShowFlags(const StringName& viewport_id, graphics::ShowFlags flags)
{
    if (const auto state = viewports.Find(viewport_id))
    {
        state->render_view.show_flags = flags;
    }
}

void EditorViewportSubsystem::SetViewportGizmoMode(const StringName& viewport_id, EGizmoMode mode)
{
    if (const auto state = viewports.Find(viewport_id))
    {
        state->gizmo_mode = mode;
    }
}

EGizmoMode EditorViewportSubsystem::GetViewportGizmoMode(const StringName& viewport_id) const
{
    return viewports.Find(viewport_id).Map([](const ViewportState& state)
    {
        return state.gizmo_mode;
    }).ValueOr(EGizmoMode::Translate);
}

void EditorViewportSubsystem::SetViewportCoordinateSpace(const StringName& viewport_id, ECoordinateSpace space)
{
    if (const auto state = viewports.Find(viewport_id))
    {
        state->coordinate_space = space;
    }
}

ECoordinateSpace EditorViewportSubsystem::GetViewportCoordinateSpace(const StringName& viewport_id) const
{
    return viewports.Find(viewport_id).Map([](const ViewportState& state)
    {
        return state.coordinate_space;
    }).ValueOr(ECoordinateSpace::World);
}

void* EditorViewportSubsystem::GetViewportTextureID(const StringName& viewport_id) const
{
    if (const auto state = viewports.Find(viewport_id))
    {
        if (state->color_texture.IsValid())
        {
            if (const auto tex_resource = render_device->GetTexture(state->color_texture))
            {
                return tex_resource->handle;
            }
        }
    }
    return nullptr;
}

Optional<const EditorCameraState&> EditorViewportSubsystem::GetViewportCamera(const StringName& viewport_id) const
{
    return viewports.Find(viewport_id).Map([](const ViewportState& state) -> const EditorCameraState&
    {
        return (state.view_mode == EViewMode::Perspective) ? state.persp_camera : state.ortho_camera;
    });
}

Optional<EditorCameraState&> EditorViewportSubsystem::GetViewportCamera(const StringName& viewport_id)
{
    return viewports.Find(viewport_id).Map([](ViewportState& state) -> EditorCameraState&
    {
        return (state.view_mode == EViewMode::Perspective) ? state.persp_camera : state.ortho_camera;
    });
}

void EditorViewportSubsystem::SetViewportViewMode(const StringName& viewport_id, EViewMode mode)
{
    if (const auto state = viewports.Find(viewport_id))
    {
        state->view_mode = mode;
        if (mode != EViewMode::Perspective)
        {
            state->ortho_camera.velocity = Vector3::Zero(); // 관성 초기화
            switch (mode)
            {
            case EViewMode::Top:    state->ortho_camera.rotation = Rotator{ -90.0_deg, 0.0_deg,   0.0_deg }; break;
            case EViewMode::Bottom: state->ortho_camera.rotation = Rotator{  90.0_deg, 0.0_deg,   0.0_deg }; break;
            case EViewMode::Front:  state->ortho_camera.rotation = Rotator{   0.0_deg, 0.0_deg, 180.0_deg }; break;
            case EViewMode::Back:   state->ortho_camera.rotation = Rotator{   0.0_deg, 0.0_deg,   0.0_deg }; break;
            case EViewMode::Right:  state->ortho_camera.rotation = Rotator{   0.0_deg, 0.0_deg,  90.0_deg }; break;
            case EViewMode::Left:   state->ortho_camera.rotation = Rotator{   0.0_deg, 0.0_deg, -90.0_deg }; break;
            default: break;
            }
        }
    }
}

EViewMode EditorViewportSubsystem::GetViewportViewMode(const StringName& viewport_id) const
{
    return viewports.Find(viewport_id).Map([](const ViewportState& state)
    {
        return state.view_mode;
    }).ValueOr(EViewMode::Perspective);
}
} // namespace se::editor
