#include "SimpleEditor/Gizmo/GizmoRenderer.h"

#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
using namespace se::math;

namespace
{
// Vector3(double) -> Vector3f(float) 축소 변환
Vector3f ToVector3f(const Vector3& v)
{
    return { static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z) };
}
} // namespace

void GizmoRenderer::SetMode(EGizmoMode in_mode)
{
    mode = in_mode;
}

void GizmoRenderer::SetHighlightAxis(EGizmoAxis axis)
{
    highlight_axis = axis;
}

void GizmoRenderer::Draw(GizmoDrawList& draw_list, const Vector3& position, const Quaternion& rotation, const graphics::RenderView& view)
{
    const float scale = ComputeScreenScale(position, view);
    if (scale < KINDA_SMALL_NUMBER) { return; }

    switch (mode)
    {
    case EGizmoMode::Translate:
        DrawTranslate(draw_list, position, rotation, scale);
        break;
    case EGizmoMode::Rotate:
        DrawRotate(draw_list, position, rotation, scale);
        break;
    case EGizmoMode::Scale:
        DrawScale(draw_list, position, rotation, scale);
        break;
    }
}

float GizmoRenderer::ComputeScreenScale(const Vector3& position, const graphics::RenderView& view)
{
    // View Space에서의 Z값(카메라 평면으로부터의 거리)을 구하는 식
    // 역행렬 곱 연산 전체를 수행하지 않고, View Matrix의 전방 축(Z축) 내적만 수행하여 성능을 최적화 함.
    const double vz = position.x * view.view_matrix[0, 2]
                    + position.y * view.view_matrix[1, 2]
                    + position.z * view.view_matrix[2, 2]
                    +              view.view_matrix[3, 2];

    const double distance = Abs(vz);
    if (distance < KINDA_SMALL_NUMBER) { return 0.0f; }

    // 원근 투영에서 Projection[1, 1] =  (1 / tan(FOV_Y / 2))
    // 거리(Distance)에 이 값을 나누어 FOV가 변하더라도 기즈모 크기가 일정하게 유지되도록 보정한다.
    const double proj_11 = view.projection_matrix[1, 1];
    if (Abs(proj_11) < KINDA_SMALL_NUMBER) { return 0.0f; }

    return static_cast<float>(distance / proj_11 * GIZMO_SCREEN_RATIO);
}

LinearColor GizmoRenderer::GetAxisColor(EGizmoAxis axis) const
{
    if (axis == highlight_axis) { return LinearColor::Yellow(); }

    switch (axis)
    {
    case EGizmoAxis::X:  return LinearColor::Red();
    case EGizmoAxis::Y:  return LinearColor::Green();
    case EGizmoAxis::Z:  return LinearColor::Blue();
    default:             return LinearColor::White();
    }
}

void GizmoRenderer::DrawTranslate(GizmoDrawList& list, const Vector3& pos, const Quaternion& rot, float scale)
{
    const Vector3 axes[3] = {
        rot.GetRightVector(),   // X
        rot.GetForwardVector(), // Y
        rot.GetUpVector()       // Z
    };
    constexpr EGizmoAxis axis_ids[3] = { EGizmoAxis::X, EGizmoAxis::Y, EGizmoAxis::Z };

    for (usize i = 0; i < 3; ++i)
    {
        const Vector3& axis = axes[i];
        const LinearColor color = GetAxisColor(axis_ids[i]);

        const float body_length = (ARROW_LENGTH - ARROW_HEAD_LENGTH) * scale;
        const float head_length = ARROW_HEAD_LENGTH * scale;

        // 솔리드 실린더 몸통
        BuildSolidCylinder(list, pos, axis, ARROW_BODY_RADIUS * scale, body_length, color, ARROW_SEGMENTS);

        // 솔리드 원뿔 머리
        const Vector3 cone_base = pos + axis * static_cast<double>(body_length);
        BuildSolidCone(list, cone_base, axis, ARROW_HEAD_RADIUS * scale, head_length, color, ARROW_SEGMENTS);
    }
}

void GizmoRenderer::DrawRotate(GizmoDrawList& list, const Vector3& pos, const Quaternion& rot, float scale)
{
    const Vector3 axes[3] = {
        rot.GetRightVector(),
        rot.GetForwardVector(),
        rot.GetUpVector()
    };
    constexpr EGizmoAxis axis_ids[3] = { EGizmoAxis::X, EGizmoAxis::Y, EGizmoAxis::Z };

    const double radius = static_cast<double>(CIRCLE_RADIUS * scale);
    const Radian angle_step = Radian{ PI_DOUBLE * 2.0 } / CIRCLE_SEGMENTS;

    for (int32 i = 0; i < 3; ++i)
    {
        const Vector3& normal = axes[i];
        const LinearColor color = GetAxisColor(axis_ids[i]);

        Vector3 tangent, bitangent;
        normal.GetOrthogonalAxes(tangent, bitangent);

        for (int32 j = 0; j < CIRCLE_SEGMENTS; ++j)
        {
            const Radian a0 = angle_step * j;
            const Radian a1 = angle_step * (j + 1);

            const Vector3 p0 = pos + (tangent * Cos(a0) + bitangent * Sin(a0)) * radius;
            const Vector3 p1 = pos + (tangent * Cos(a1) + bitangent * Sin(a1)) * radius;

            list.AddLine(
                { .position = ToVector3f(p0), .color = color },
                { .position = ToVector3f(p1), .color = color }
            );
        }
    }
}

void GizmoRenderer::DrawScale(GizmoDrawList& list, const Vector3& pos, const Quaternion& rot, float scale)
{
    const Vector3 axes[3] = {
        rot.GetRightVector(),
        rot.GetForwardVector(),
        rot.GetUpVector()
    };
    constexpr EGizmoAxis axis_ids[3] = { EGizmoAxis::X, EGizmoAxis::Y, EGizmoAxis::Z };

    for (int32 i = 0; i < 3; ++i)
    {
        const Vector3& axis = axes[i];
        const LinearColor color = GetAxisColor(axis_ids[i]);
        const double line_len = static_cast<double>(SCALE_LINE_LENGTH * scale);
        const Vector3 endpoint = pos + axis * line_len;

        // 축 라인
        list.AddLine(
            { .position = ToVector3f(pos), .color = color },
            { .position = ToVector3f(endpoint), .color = color }
        );

        // 끝점 큐브 (모든 축 큐브에 동일한 로컬 좌표계 사용)
        BuildSolidCube(list, endpoint, SCALE_CUBE_HALF * scale, axes[0], axes[2], axes[1], color);
    }

    // 중앙 큐브 (전체 균등 스케일)
    const LinearColor center_color = GetAxisColor(EGizmoAxis::All);
    BuildSolidCube(list, pos, SCALE_CUBE_HALF * scale * 0.8f, axes[0], axes[2], axes[1], center_color);
}

void GizmoRenderer::BuildSolidCylinder(
    GizmoDrawList& list, const Vector3& base_center, const Vector3& axis_dir,
    float radius, float height, const LinearColor& color, int32 segments
)
{
    Vector3 tangent, bitangent;
    axis_dir.GetOrthogonalAxes(tangent, bitangent);

    const Vector3 top_center = base_center + axis_dir * static_cast<double>(height);
    const Radian angle_step = Radian{ PI_DOUBLE * 2.0 } / segments;

    for (int32 i = 0; i < segments; ++i)
    {
        const Radian a0 = angle_step * i;
        const Radian a1 = angle_step * (i + 1);

        const Vector3 offset0 = (tangent * Cos(a0) + bitangent * Sin(a0)) * radius;
        const Vector3 offset1 = (tangent * Cos(a1) + bitangent * Sin(a1)) * radius;

        const Vector3 b0 = base_center + offset0;
        const Vector3 b1 = base_center + offset1;
        const Vector3 t0 = top_center + offset0;
        const Vector3 t1 = top_center + offset1;

        // 실린더의 한 면을 구성하는 두 개의 삼각형(Quad) 추가
        list.AddTriangle(
            { .position = ToVector3f(b0), .color = color },
            { .position = ToVector3f(b1), .color = color },
            { .position = ToVector3f(t0), .color = color }
        );
        list.AddTriangle(
            { .position = ToVector3f(b1), .color = color },
            { .position = ToVector3f(t1), .color = color },
            { .position = ToVector3f(t0), .color = color }
        );
    }
}

void GizmoRenderer::BuildSolidCone(
    GizmoDrawList& list, const Vector3& base_center, const Vector3& axis_dir,
    float radius, float height, const LinearColor& color, int32 segments
)
{
    Vector3 tangent, bitangent;
    axis_dir.GetOrthogonalAxes(tangent, bitangent);

    const Vector3 tip = base_center + axis_dir * static_cast<double>(height);
    const Radian angle_step = Radian{ PI_DOUBLE * 2.0 } / segments;

    for (int32 i = 0; i < segments; ++i)
    {
        const Radian a0 = angle_step * i;
        const Radian a1 = angle_step * (i + 1);

        const Vector3 p0 = base_center + (tangent * Cos(a0) + bitangent * Sin(a0)) * radius;
        const Vector3 p1 = base_center + (tangent * Cos(a1) + bitangent * Sin(a1)) * radius;

        list.AddTriangle(
            { .position = ToVector3f(tip), .color = color },
            { .position = ToVector3f(p0),  .color = color },
            { .position = ToVector3f(p1),  .color = color }
        );
    }
}

void GizmoRenderer::BuildSolidCube(
    GizmoDrawList& list, const Vector3& center, float half_extent,
    const Vector3& right, const Vector3& up, const Vector3& forward,
    const LinearColor& color
)
{
    const double h = static_cast<double>(half_extent);

    // 큐브를 구성하는 8개의 로컬 꼭짓점 계산
    const Vector3 corners[8] = {
        center + (-right - forward - up) * h,  // 0: 좌하단 후면
        center + ( right - forward - up) * h,  // 1: 우하단 후면
        center + ( right + forward - up) * h,  // 2: 우하단 전면
        center + (-right + forward - up) * h,  // 3: 좌하단 전면
        center + (-right - forward + up) * h,  // 4: 좌상단 후면
        center + ( right - forward + up) * h,  // 5: 우상단 후면
        center + ( right + forward + up) * h,  // 6: 우상단 전면
        center + (-right + forward + up) * h,  // 7: 좌상단 전면
    };

    // 6개의 면 정의 (반시계 방향으로 꼭짓점 인덱스 정렬)
    struct Face { int32 i0, i1, i2, i3; Vector3 normal; };
    const Face faces[6] = {
        { 1, 2, 6, 5,  right   },  // +X
        { 3, 0, 4, 7, -right   },  // -X
        { 2, 3, 7, 6,  forward },  // +Y
        { 0, 1, 5, 4, -forward },  // -Y
        { 4, 5, 6, 7,  up      },  // +Z
        { 0, 3, 2, 1, -up      },  // -Z
    };

    for (const Face& face : faces)
    {
        // 한 면을 두 개의 삼각형으로 분할
        list.AddTriangle(
            { .position = ToVector3f(corners[face.i0]), .color = color },
            { .position = ToVector3f(corners[face.i1]), .color = color },
            { .position = ToVector3f(corners[face.i2]), .color = color }
        );
        list.AddTriangle(
            { .position = ToVector3f(corners[face.i0]), .color = color },
            { .position = ToVector3f(corners[face.i2]), .color = color },
            { .position = ToVector3f(corners[face.i3]), .color = color }
        );
    }
}
} // namespace se::editor
