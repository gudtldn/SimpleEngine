#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Debug/DebugDraw.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"


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
            viewports[active_camera_viewport].camera.velocity = Vector3::Zero();

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
        EditorCameraState& camera = viewports[active_camera_viewport].camera;

        // 마우스 회전 yaw(Z축), pitch(X축) | TODO: 나중에 쿼터니언으로 수정
        const Vector2f mouse_delta = input_subsystem->GetMouseDelta();
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

        // 스크롤로 이동 속도를 조절
        const float scroll = input_subsystem->GetMouseWheel().y;
        if (scroll != 0.0f)
        {
            camera.move_speed *= (scroll > 0.0f) ? 1.1 : (1.0 / 1.1);
            camera.move_speed = Clamp(camera.move_speed, 0.1, 1000.0);
        }

        // 카메라가 활성화 되어있는 동안, 마우스를 last_pos에 고정
        input_subsystem->SetLocalMousePosition(last_mouse_pos);
    }

    // 월드 기준 좌표축을 매 프레임 그리기
    DrawDebugWorldAxes();
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

void EditorViewportSubsystem::SetViewportCoordinateSpace(const StringName& viewport_id, ECoordinateSpace space)
{
    if (const auto state = viewports.Find(viewport_id))
    {
        state->coordinate_space = space;
    }
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

EGizmoMode EditorViewportSubsystem::GetViewportGizmoMode(const StringName& viewport_id) const
{
    return viewports.Find(viewport_id).Map([](const ViewportState& state)
    {
        return state.gizmo_mode;
    }).ValueOr(EGizmoMode::Translate);
}

ECoordinateSpace EditorViewportSubsystem::GetViewportCoordinateSpace(const StringName& viewport_id) const
{
    return viewports.Find(viewport_id).Map([](const ViewportState& state)
    {
        return state.coordinate_space;
    }).ValueOr(ECoordinateSpace::World);
}

Optional<const EditorCameraState&> EditorViewportSubsystem::GetViewportCamera(const StringName& viewport_id) const
{
    return viewports.Find(viewport_id).Map([](const ViewportState& state) -> const EditorCameraState&
    {
        return state.camera;
    });
}

Optional<EditorCameraState&> EditorViewportSubsystem::GetViewportCamera(const StringName& viewport_id)
{
    return viewports.Find(viewport_id).Map([](ViewportState& state) -> EditorCameraState&
    {
        return state.camera;
    });
}
} // namespace se::editor
