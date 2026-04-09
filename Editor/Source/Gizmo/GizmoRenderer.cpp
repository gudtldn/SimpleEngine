// NOLINTBEGIN(*-use-designated-initializers)

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

void GizmoRenderer::Draw(GizmoDrawList& draw_list, const Quaternion& rotation)
{
    switch (mode)
    {
    case EGizmoMode::Translate:
        DrawTranslate(draw_list, rotation);
        break;
    case EGizmoMode::Rotate:
        DrawRotate(draw_list, rotation);
        break;
    case EGizmoMode::Scale:
        DrawScale(draw_list, rotation);
        break;
    }
}

float GizmoRenderer::ComputeScreenScale(const Vector3& position, const graphics::RenderView& view)
{
    // 원근 투영에서 Projection[1, 1]은 (1 / tan(FOV_Y / 2))을 의미
    const double proj_11 = view.projection_matrix[1, 1];
    if (Abs(proj_11) < KINDA_SMALL_NUMBER) { return 0.0f; }

    // 투영 행렬의 [2, 3] 성분을 확인하여 직교(0)와 원근(-1) 투영을 판별
    const double proj_23 = view.projection_matrix[2, 3];
    if (Abs(proj_23) < KINDA_SMALL_NUMBER)
    {
        // 직교 투영인 경우, 카메라와의 거리에 무관하게 가시 영역 높이에 비례한 고정 스케일을 반환
        return static_cast<float>(GIZMO_SCREEN_RATIO / proj_11);
    }

    // 원근 투영인 경우, View Space에서의 Z값(카메라 평면으로부터의 거리)을 추출
    // 역행렬 곱 연산 전체를 수행하지 않고, View Matrix의 전방 축(Z축) 내적만 수행하여 성능을 최적화한다.
    const double vz = (position.x * view.view_matrix[0, 2])
                    + (position.y * view.view_matrix[1, 2])
                    + (position.z * view.view_matrix[2, 2])
                    +               view.view_matrix[3, 2];

    const double distance = Abs(vz);
    if (distance < KINDA_SMALL_NUMBER) { return 0.0f; }

    // 거리(distance)에 proj_11 값을 나누어 FOV가 변하더라도 기즈모 크기가 일정하게 유지되도록 보정
    return static_cast<float>(distance / proj_11 * GIZMO_SCREEN_RATIO);
}

LinearColor GizmoRenderer::GetAxisColor(EGizmoAxis axis) const
{
    if (axis == highlight_axis) { return LinearColor::White(); }

    switch (axis)
    {
    case EGizmoAxis::X:  return LinearColor::Red();
    case EGizmoAxis::Y:  return LinearColor::Green();
    case EGizmoAxis::Z:  return LinearColor::Blue();
    case EGizmoAxis::XY: return LinearColor::Blue();  // XY 평면의 수직축 = Z
    case EGizmoAxis::XZ: return LinearColor::Green(); // XZ 평면의 수직축 = Y
    case EGizmoAxis::YZ: return LinearColor::Red();   // YZ 평면의 수직축 = X
    default:             return LinearColor::White();
    }
}

void GizmoRenderer::DrawTranslate(GizmoDrawList& list, const Quaternion& rot)
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

        constexpr float BODY_LENGTH = AXIS_LENGTH - TRANSLATE_HEAD_LENGTH;

        // 솔리드 실린더 몸통
        BuildSolidCylinder(list, Vector3::Zero(), axis, AXIS_BODY_RADIUS, BODY_LENGTH, color, AXIS_SEGMENTS);

        // 솔리드 원뿔 머리
        const Vector3 cone_base = axis * static_cast<double>(BODY_LENGTH);
        BuildSolidCone(list, cone_base, axis, TRANSLATE_HEAD_RADIUS, TRANSLATE_HEAD_LENGTH, color, AXIS_SEGMENTS);
    }

    // 기준점 구체
    BuildSolidSphere(list, Vector3::Zero(), ORIGIN_SPHERE_RADIUS, GetAxisColor(EGizmoAxis::All), SPHERE_RINGS, SPHERE_SECTORS);

    // XY/XZ/YZ 평면 핸들 (채운 쿼드)
    struct PlaneInfo { int32 a0; int32 a1; EGizmoAxis axis; };
    constexpr PlaneInfo planes[3] = {
        { 0, 1, EGizmoAxis::XY },
        { 0, 2, EGizmoAxis::XZ },
        { 1, 2, EGizmoAxis::YZ },
    };

    constexpr double OFFSET = static_cast<double>(PLANE_HANDLE_OFFSET);
    constexpr double LENGTH = static_cast<double>(PLANE_HANDLE_LENGTH);

    for (const auto& [a0, a1, plane_axis] : planes)
    {
        const LinearColor color = GetAxisColor(plane_axis);

        const Vector3 corner = axes[a0] * OFFSET + axes[a1] * OFFSET;
        const Vector3 edge0  = axes[a0] * (OFFSET - LENGTH) + axes[a1] * OFFSET;
        const Vector3 edge1  = axes[a0] * OFFSET + axes[a1] * (OFFSET - LENGTH);
        const Vector3 inner  = axes[a0] * (OFFSET - LENGTH) + axes[a1] * (OFFSET - LENGTH);

        list.AddTriangle(
            { .position = ToVector3f(inner), .color = color },
            { .position = ToVector3f(edge0), .color = color },
            { .position = ToVector3f(corner), .color = color }
        );
        list.AddTriangle(
            { .position = ToVector3f(inner), .color = color },
            { .position = ToVector3f(corner), .color = color },
            { .position = ToVector3f(edge1), .color = color }
        );
    }
}

void GizmoRenderer::DrawRotate(GizmoDrawList& list, const Quaternion& rot)
{
    const Vector3 axes[3] = {
        rot.GetRightVector(),
        rot.GetForwardVector(),
        rot.GetUpVector()
    };
    constexpr EGizmoAxis axis_ids[3] = { EGizmoAxis::X, EGizmoAxis::Y, EGizmoAxis::Z };

    constexpr double RADIUS = static_cast<double>(CIRCLE_RADIUS);
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

            const Vector3 p0 = (tangent * Cos(a0) + bitangent * Sin(a0)) * RADIUS;
            const Vector3 p1 = (tangent * Cos(a1) + bitangent * Sin(a1)) * RADIUS;

            list.AddLine(
                { .position = ToVector3f(p0), .color = color },
                { .position = ToVector3f(p1), .color = color }
            );
        }
    }
}

void GizmoRenderer::DrawScale(GizmoDrawList& list, const Quaternion& rot)
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

        // 솔리드 실린더 몸통
        BuildSolidCylinder(list, Vector3::Zero(), axis, AXIS_BODY_RADIUS, AXIS_LENGTH, color, AXIS_SEGMENTS);

        // 끝점 큐브
        const Vector3 endpoint = axis * static_cast<double>(AXIS_LENGTH);
        BuildSolidCube(list, endpoint, SCALE_CUBE_HALF, axes[0], axes[2], axes[1], color);
    }

    // 중앙 큐브 (전체 균등 스케일)
    const LinearColor center_color = GetAxisColor(EGizmoAxis::All);
    BuildSolidCube(list, Vector3::Zero(), SCALE_CUBE_HALF * 0.8f, axes[0], axes[2], axes[1], center_color);
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

void GizmoRenderer::BuildSolidSphere(GizmoDrawList& list, const Vector3& center, float radius, const LinearColor& color, int32 rings, int32 sectors)
{
    const double r = static_cast<double>(radius);

    // UV 구체 정점 위치 계산 (Z-up)
    auto get_point = [&](int32 ring, int32 sector) -> Vector3
    {
        const Radian phi = Radian{ PI_DOUBLE } * ring / rings;
        const Radian theta = Radian{ PI_DOUBLE * 2.0 } * sector / sectors;

        return center + Vector3{
            Sin(phi) * Cos(theta) * r,
            Sin(phi) * Sin(theta) * r,
            Cos(phi) * r
        };
    };

    for (int32 i = 0; i < rings; ++i)
    {
        for (int32 j = 0; j < sectors; ++j)
        {
            const int32 j_next = (j + 1) % sectors;

            const Vector3 p00 = get_point(i, j);
            const Vector3 p10 = get_point(i + 1, j);
            const Vector3 p01 = get_point(i, j_next);
            const Vector3 p11 = get_point(i + 1, j_next);

            if (i == 0)
            {
                // 상단 극점: 삼각형 팬
                list.AddTriangle(
                    { .position = ToVector3f(p00), .color = color },
                    { .position = ToVector3f(p10), .color = color },
                    { .position = ToVector3f(p11), .color = color }
                );
            }
            else if (i == rings - 1)
            {
                // 하단 극점: 삼각형 팬
                list.AddTriangle(
                    { .position = ToVector3f(p00), .color = color },
                    { .position = ToVector3f(p01), .color = color },
                    { .position = ToVector3f(p10), .color = color }
                );
            }
            else
            {
                // 일반 쿼드: 두 삼각형
                list.AddTriangle(
                    { .position = ToVector3f(p00), .color = color },
                    { .position = ToVector3f(p10), .color = color },
                    { .position = ToVector3f(p11), .color = color }
                );
                list.AddTriangle(
                    { .position = ToVector3f(p00), .color = color },
                    { .position = ToVector3f(p11), .color = color },
                    { .position = ToVector3f(p01), .color = color }
                );
            }
        }
    }
}
} // namespace se::editor

// NOLINTEND(*-use-designated-initializers)
