#include "SimpleEditor/UI/EditorViewportSubsystem.h"

#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


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
        for (const ViewportRenderInfo& info : viewport_data | std::views::values)
        {
            if (info.color_texture)
            {
                render_device->DestroyTexture(info.color_texture);
            }
        }
    }
    viewport_data.Clear();
    viewport_cameras.Clear();
    render_device = nullptr;
    input_subsystem = nullptr;
}

void EditorViewportSubsystem::Update(float delta_time)
{
    // 우클릭으로 호버된 뷰포트의 카메라 제어를 활성화
    if (input_subsystem->IsMouseButtonPressed(EMouseButton::Right))
    {
        for (const auto& [id, info] : viewport_data)
        {
            if (info.is_hovered)
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
            input_subsystem->SetLocalMousePosition(last_mouse_pos);
            input_subsystem->SetRelativeMouseMode(false);

            active_camera_viewport = StringName::None;
            last_mouse_pos = Vector2f::Zero();
        }
    }

    // 활성 카메라 입력 처리
    if (active_camera_viewport != StringName::None && input_subsystem->IsMouseButtonDown(EMouseButton::Right))
    {
        EditorCameraState& camera = viewport_cameras[active_camera_viewport];

        // 마우스 회전 yaw(Z축), pitch(X축) | TODO: 나중에 쿼터니언으로 수정
        const Vector2f mouse_delta = input_subsystem->GetMouseDelta();
        camera.rotation.yaw -= Degree{ mouse_delta.x * camera.look_sensitivity };
        camera.rotation.yaw = Degree{ Fmod(*camera.rotation.yaw, 360.0) };
        camera.rotation.pitch -= Degree{ mouse_delta.y * camera.look_sensitivity };
        camera.rotation.pitch = Degree{ Clamp(*camera.rotation.pitch, -89.0, 89.0) };

        // WASD/QE 이동
        const Vector3 forward = camera.rotation.GetForwardVector();
        const Vector3 right = camera.rotation.GetRightVector();

        Vector3 move_dir = Vector3::Zero();
        if (input_subsystem->IsKeyDown(EKeyCode::W)) { move_dir += forward; }
        if (input_subsystem->IsKeyDown(EKeyCode::S)) { move_dir -= forward; }
        if (input_subsystem->IsKeyDown(EKeyCode::D)) { move_dir += right; }
        if (input_subsystem->IsKeyDown(EKeyCode::A)) { move_dir -= right; }
        if (input_subsystem->IsKeyDown(EKeyCode::E)) { move_dir += Vector3::Up(); }
        if (input_subsystem->IsKeyDown(EKeyCode::Q)) { move_dir -= Vector3::Up(); }

        if (!move_dir.IsNearlyZero())
        {
            camera.position += move_dir.GetNormalized() * (camera.move_speed * delta_time);
        }

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

    // 모든 뷰포트의 RenderView를 카메라 상태로부터 갱신
    for (auto& [id, info] : viewport_data)
    {
        if (info.render_view.width == 0 || info.render_view.height == 0)
        {
            continue;
        }
        const EditorCameraState& camera = viewport_cameras[id];
        info.render_view = camera.ComputeRenderView(info.render_view.width, info.render_view.height);
    }
}

void EditorViewportSubsystem::UpdateViewportSize(const StringName& viewport_id, uint32 new_width, uint32 new_height)
{
    if (new_width == 0 || new_height == 0)
    {
        if (const Optional data_opt = viewport_data.Find(viewport_id))
        {
            ViewportRenderInfo& info = data_opt.Value();
            if (info.color_texture)
            {
                render_device->DestroyTexture(
                    std::exchange(info.color_texture, {})
                );
            }
        }
        return;
    }

    // 텍스처가 없거나, 크기가 변경된경우 재생성
    ViewportRenderInfo& info = viewport_data[viewport_id];
    if (
        !info.color_texture.IsValid()
        || info.render_view.width != new_width
        || info.render_view.height != new_height
    )
    {
        if (info.color_texture)
        {
            render_device->DestroyTexture(info.color_texture);
        }

        info.color_texture = render_device->CreateTexture({
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = new_width,
            .height = new_height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        });
        info.render_view.width = new_width;
        info.render_view.height = new_height;
        info.color_target_name = viewport_id;
        info.depth_target_name = String::Format("{}_Depth", viewport_id);
    }
}

void EditorViewportSubsystem::UpdateViewportFocus(const StringName& viewport_id, bool focused, bool hovered)
{
    if (const Optional data_opt = viewport_data.Find(viewport_id))
    {
        ViewportRenderInfo& info = data_opt.Value();
        info.is_focused = focused;
        info.is_hovered = hovered;
    }
}

void* EditorViewportSubsystem::GetViewportTextureID(const StringName& viewport_id) const
{
    if (const Optional data_opt = viewport_data.Find(viewport_id))
    {
        const ViewportRenderInfo& info = data_opt.Value();
        if (info.color_texture.IsValid())
        {
            if (const auto tex_resource = render_device->GetTexture(info.color_texture))
            {
                return tex_resource->handle;
            }
        }
    }
    return nullptr;
}
} // namespace se::editor
