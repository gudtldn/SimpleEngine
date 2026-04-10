#include "SimpleEditor/Gizmo/GizmoInteraction.h"

#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
void GizmoInteraction::BeginDrag(
    EGizmoMode in_mode,
    EGizmoAxis in_axis,
    bool in_local,
    const Vector2f& in_cursor_pos,
    const Vector3& in_gizmo_center,
    const Quaternion& in_gizmo_rotation,
    const graphics::RenderView& in_view
)
{
    dragging = true;
    is_local = in_local;
    active_mode = in_mode;
    active_axis = in_axis;
    drag_gizmo_center = in_gizmo_center;
    drag_gizmo_rotation = in_gizmo_rotation;

    const Ray ray = in_view.DeprojectToRay(in_cursor_pos);

    switch (active_mode)
    {
    case EGizmoMode::Translate:
    {
        // All: 카메라를 향하는 평면에서 자유 이동
        if (active_axis == EGizmoAxis::All)
        {
            const Vector3 normal = (ray.origin - drag_gizmo_center).GetNormalized();
            double t = 0.0;
            if (ray.IntersectPlane(drag_gizmo_center, normal, t))
            {
                drag_start_t = t;
                drag_plane_normal = normal;
                drag_start_vector = ray.GetPoint(t);
            }
        }

        // 평면 핸들(XY, XZ, YZ)
        else if (
            active_axis == EGizmoAxis::XY
            || active_axis == EGizmoAxis::XZ
            || active_axis == EGizmoAxis::YZ
        )
        {
            const Vector3 normal = GetPlaneNormal(active_axis);
            double t = 0.0;
            if (ray.IntersectPlane(drag_gizmo_center, normal, t))
            {
                drag_start_t = t;
                drag_plane_normal = normal;
                drag_start_vector = ray.GetPoint(t);
            }
        }

        // 단일 축(X, Y, Z)
        else
        {
            const Vector3 axis_dir = GetAxisDirection(active_axis);
            drag_start_t = ray.ClosestParameterOnLine(drag_gizmo_center, axis_dir);
        }
        break;
    }

    case EGizmoMode::Scale:
    {
        // All: 화면 커서 위치를 기록 (frame-to-frame delta로 스케일 계산)
        if (active_axis == EGizmoAxis::All)
        {
            drag_start_vector = Vector3{
                static_cast<double>(in_cursor_pos.x),
                static_cast<double>(in_cursor_pos.y),
                0.0
            };
        }

        // 평면 핸들(XY, XZ, YZ)
        else if (
            active_axis == EGizmoAxis::XY
            || active_axis == EGizmoAxis::XZ
            || active_axis == EGizmoAxis::YZ
        )
        {
            const Vector3 normal = GetPlaneNormal(active_axis);
            double t = 0.0;
            if (ray.IntersectPlane(drag_gizmo_center, normal, t))
            {
                drag_start_t = t;
                drag_plane_normal = normal;
                drag_start_vector = ray.GetPoint(t);
            }
        }

        // 단일 축(X, Y, Z)
        else
        {
            const Vector3 axis_dir = GetAxisDirection(active_axis);
            drag_start_t = ray.ClosestParameterOnLine(drag_gizmo_center, axis_dir);
        }
        break;
    }
    case EGizmoMode::Rotate:
    {
        // 회전축 방향을 기록 (sign flip 판정용: 카메라와의 facing 방향 비교)
        drag_plane_normal = GetAxisDirection(active_axis);
        const Vector2f center_screen = in_view.ProjectWorldToScreen(drag_gizmo_center);
        const double dx = static_cast<double>(in_cursor_pos.x - center_screen.x);
        const double dy = static_cast<double>(in_cursor_pos.y - center_screen.y);
        drag_start_angle = math::Atan2(dy, dx);
        break;
    }
    }
}

GizmoInteraction::DragResult GizmoInteraction::UpdateDrag(const Vector2f& cursor_pos, const graphics::RenderView& view)
{
    if (!dragging) { return {}; }

    const Ray ray = view.DeprojectToRay(cursor_pos);
    switch (active_mode)
    {
    case EGizmoMode::Translate: return UpdateTranslation(ray);
    case EGizmoMode::Rotate:    return UpdateRotation(cursor_pos, view);
    case EGizmoMode::Scale:     return UpdateScale(ray, cursor_pos);
    }

    return {};
}

void GizmoInteraction::EndDrag()
{
    dragging = false;
    active_axis = EGizmoAxis::None;
    drag_start_t = 0.0;
    drag_start_angle = 0.0_rad;
}

void GizmoInteraction::OffsetDragReference(const Vector3& offset)
{
    drag_start_vector += offset;

    // 단일 축(X/Y/Z): t 파라미터 보정
    if (active_axis == EGizmoAxis::X || active_axis == EGizmoAxis::Y || active_axis == EGizmoAxis::Z)
    {
        const Vector3 axis_dir = GetAxisDirection(active_axis);
        drag_start_t += offset.Dot(axis_dir);
    }
}

GizmoInteraction::DragResult GizmoInteraction::UpdateTranslation(const Ray& ray)
{
    DragResult result;

    // 평면 이동
    if (
        active_axis == EGizmoAxis::XY
        || active_axis == EGizmoAxis::XZ
        || active_axis == EGizmoAxis::YZ
        || active_axis == EGizmoAxis::All
    )
    {
        double t = 0.0;
        if (ray.IntersectPlane(drag_gizmo_center, drag_plane_normal, t))
        {
            const Vector3 hit_point = ray.GetPoint(t);

            // 이전 프레임의 히트 위치(drag_start_vector)와 현재 히트 위치의 차이를 Delta로 반환
            result.translation_delta = hit_point - drag_start_vector;
            drag_start_vector = hit_point; // 다음 프레임의 기준점 갱신
            drag_gizmo_center = drag_gizmo_center + result.translation_delta;
        }
    }

    // 단일 축 이동
    else
    {
        const Vector3 axis_dir = GetAxisDirection(active_axis);
        const double current_t = ray.ClosestParameterOnLine(drag_gizmo_center, axis_dir);

        // 투영 거리 t의 차이만큼 해당 축 방향으로 이동
        const double delta_t = current_t - drag_start_t;
        result.translation_delta = axis_dir * delta_t;

        // center가 이동하면 ClosestParameterOnLine의 t가 정확히 delta_t만큼 시프트되므로
        // drag_start_t를 유지하면 다음 프레임에서 상쇄된다 (리셋하면 점프 발생)
        drag_gizmo_center = drag_gizmo_center + result.translation_delta;
    }

    return result;
}

GizmoInteraction::DragResult GizmoInteraction::UpdateScale(const Ray& ray, const Vector2f& cursor_pos)
{
    DragResult result;

    if (active_axis == EGizmoAxis::All)
    {
        // 화면 커서 이동량으로 균등 스케일 계산 (오른쪽/위 = 확대, 왼쪽/아래 = 축소)
        const double dx = static_cast<double>(cursor_pos.x) - drag_start_vector.x;
        const double dy = static_cast<double>(cursor_pos.y) - drag_start_vector.y;
        drag_start_vector.x = static_cast<double>(cursor_pos.x);
        drag_start_vector.y = static_cast<double>(cursor_pos.y);

        // dx - dy: 화면 X+ = 오른쪽(확대), 화면 Y- = 위쪽(확대)
        const double delta = (dx - dy) * SCALE_SENSITIVITY;
        result.scale_delta = Vector3{ delta, delta, delta };
    }

    // 평면 스케일: 평면 교차 거리 변화 -> 두 축 동시 스케일
    else if (
        active_axis == EGizmoAxis::XY
        || active_axis == EGizmoAxis::XZ
        || active_axis == EGizmoAxis::YZ
    )
    {
        double t = 0.0;
        if (ray.IntersectPlane(drag_gizmo_center, drag_plane_normal, t))
        {
            const Vector3 hit_point = ray.GetPoint(t);
            const Vector3 delta = hit_point - drag_start_vector;
            drag_start_vector = hit_point;

            // 두 축 projection의 합산 -> 균등 스케일 (고정 비율)
            const auto [a0, a1] = [&] -> std::pair<EGizmoAxis, EGizmoAxis>
            {
                switch (active_axis)
                {
                case EGizmoAxis::XY: return { EGizmoAxis::X, EGizmoAxis::Y };
                case EGizmoAxis::XZ: return { EGizmoAxis::X, EGizmoAxis::Z };
                case EGizmoAxis::YZ: return { EGizmoAxis::Y, EGizmoAxis::Z };
                default:             return { EGizmoAxis::X, EGizmoAxis::Y };
                }
            }();

            const double s0 = delta.Dot(GetAxisDirection(a0));
            const double s1 = delta.Dot(GetAxisDirection(a1));
            const double uniform = s0 + s1;

            if (active_axis == EGizmoAxis::XY)
            {
                result.scale_delta = Vector3{ uniform, uniform, 0.0 };
            }
            else if (active_axis == EGizmoAxis::XZ)
            {
                result.scale_delta = Vector3{ uniform, 0.0, uniform };
            }
            else // YZ
            {
                result.scale_delta = Vector3{ 0.0, uniform, uniform };
            }
        }
    }

    // 단일 축 스케일
    else
    {
        const Vector3 axis_dir = GetAxisDirection(active_axis);
        const double current_t = ray.ClosestParameterOnLine(drag_gizmo_center, axis_dir);
        const double delta_t = current_t - drag_start_t;
        drag_start_t = current_t;

        switch (active_axis)
        {
        case EGizmoAxis::X: result.scale_delta = Vector3{ delta_t, 0.0, 0.0 }; break;
        case EGizmoAxis::Y: result.scale_delta = Vector3{ 0.0, delta_t, 0.0 }; break;
        case EGizmoAxis::Z: result.scale_delta = Vector3{ 0.0, 0.0, delta_t }; break;
        default: break;
        }
    }

    return result;
}

GizmoInteraction::DragResult GizmoInteraction::UpdateRotation(const Vector2f& cursor_pos, const graphics::RenderView& view)
{
    DragResult result;

    // 화면 공간에서 기즈모 중심 대비 커서의 atan2 각도를 계산
    const Vector2f center_screen = view.ProjectWorldToScreen(drag_gizmo_center);
    const double dx = static_cast<double>(cursor_pos.x - center_screen.x);
    const double dy = static_cast<double>(cursor_pos.y - center_screen.y);
    const Radian current_angle = math::Atan2(dy, dx);

    // frame-to-frame 각도 변화량 (±π 래핑 처리)
    Radian delta_angle = current_angle - drag_start_angle;
    if (delta_angle.value > math::PI_DOUBLE) { delta_angle = delta_angle - Radian<double>{ 2.0 * math::PI_DOUBLE }; }
    if (delta_angle.value < -math::PI_DOUBLE) { delta_angle = delta_angle + Radian<double>{ 2.0 * math::PI_DOUBLE }; }
    drag_start_angle = current_angle;

    // 회전축이 카메라를 향하면 화면 CW = 3D 음수 회전 (right-hand rule)
    // 반대면 화면 CW = 3D 양수 회전
    const Matrix4x4 inv_view = view.view_matrix.Inverse();
    const Vector3 camera_pos = Vector3{ inv_view[3, 0], inv_view[3, 1], inv_view[3, 2] };
    const double facing = drag_plane_normal.Dot((camera_pos - drag_gizmo_center).GetNormalized());
    if (facing >= 0.0) { delta_angle = -delta_angle; }

    result.rotation_delta = Quaternion::FromAxisAngle(GetBasisAxis(active_axis), delta_angle);
    result.is_local_rotation = is_local;

    return result;
}

Vector3 GizmoInteraction::GetAxisDirection(EGizmoAxis axis) const
{
    switch (axis)
    {
    case EGizmoAxis::X: return drag_gizmo_rotation.GetRightVector();   // +X
    case EGizmoAxis::Y: return drag_gizmo_rotation.GetForwardVector(); // +Y
    case EGizmoAxis::Z: return drag_gizmo_rotation.GetUpVector();      // +Z
    default:            return Vector3::Zero();
    }
}

Vector3 GizmoInteraction::GetBasisAxis(EGizmoAxis axis)
{
    switch (axis)
    {
    case EGizmoAxis::X: return Vector3::Right();
    case EGizmoAxis::Y: return Vector3::Forward();
    case EGizmoAxis::Z: return Vector3::Up();
    default:            return Vector3::Up();
    }
}

Vector3 GizmoInteraction::GetPlaneNormal(EGizmoAxis axis) const
{
    switch (axis)
    {
    case EGizmoAxis::XY:  return drag_gizmo_rotation.GetUpVector();      // XY 평면에 수직인 Z축 방향
    case EGizmoAxis::XZ:  return drag_gizmo_rotation.GetForwardVector(); // XZ 평면에 수직인 Y축 방향
    case EGizmoAxis::YZ:  return drag_gizmo_rotation.GetRightVector();   // YZ 평면에 수직인 X축 방향
    default:              return drag_gizmo_rotation.GetUpVector();
    }
}
} // namespace se::editor
