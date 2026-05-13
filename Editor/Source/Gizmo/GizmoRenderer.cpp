// NOLINTBEGIN(*-use-designated-initializers, *-isolate-declaration)
// ReSharper disable CppDFAConstantParameter

#include "SimpleEditor/Gizmo/GizmoRenderer.h"

#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
using namespace se::math;

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
        DrawRotate(draw_list, rotation, draw_list.GetDirectionToWidget());
        break;
    case EGizmoMode::Scale:
        DrawScale(draw_list, rotation);
        break;
    }
}

f64 GizmoRenderer::ComputeScreenScale(const Vector3& position, const RenderView& view)
{
    // 원근 투영에서 Projection[1, 1]은 (1 / tan(FOV_Y / 2))을 의미
    const f64 proj_11 = view.projection_matrix[1, 1];
    if (Abs(proj_11) < KINDA_SMALL_NUMBER) { return 0.0; }

    // 투영 행렬의 [2, 3] 성분을 확인하여 직교(0)와 원근(-1) 투영을 판별
    const f64 proj_23 = view.projection_matrix[2, 3];
    if (Abs(proj_23) < KINDA_SMALL_NUMBER)
    {
        // 직교 투영인 경우, 카메라와의 거리에 무관하게 가시 영역 높이에 비례한 고정 스케일을 반환
        return GIZMO_SCREEN_RATIO / proj_11;
    }

    // 원근 투영인 경우, View Space에서의 Z값(카메라 평면으로부터의 거리)을 추출
    // 역행렬 곱 연산 전체를 수행하지 않고, View Matrix의 전방 축(Z축) 내적만 수행하여 성능을 최적화한다.
    const f64 vz = (position.x * view.view_matrix[0, 2])
                    + (position.y * view.view_matrix[1, 2])
                    + (position.z * view.view_matrix[2, 2])
                    +               view.view_matrix[3, 2];

    const f64 distance = Abs(vz);
    if (distance < KINDA_SMALL_NUMBER) { return 0.0; }

    // 거리(distance)에 proj_11 값을 나누어 FOV가 변하더라도 기즈모 크기가 일정하게 유지되도록 보정
    return distance / proj_11 * GIZMO_SCREEN_RATIO;
}

void GizmoRenderer::DrawTranslate(GizmoDrawList& list, const Quaternion& rot)
{
    const Vector3 axes[3] = {
        rot.GetRightVector(),   // X
        rot.GetForwardVector(), // Y
        rot.GetUpVector()       // Z
    };
    constexpr EGizmoAxis AXIS_IDS[3] = { EGizmoAxis::X, EGizmoAxis::Y, EGizmoAxis::Z };

    for (usize i = 0; i < 3; ++i)
    {
        const Vector3& axis = axes[i];
        const LinearColor color = GetAxisColor(AXIS_IDS[i]);

        constexpr f64 BODY_LENGTH = AXIS_LENGTH - TRANSLATE_HEAD_LENGTH;

        // Pick ID 설정
        list.SetPickId(EncodePickID(AXIS_IDS[i]));

        // 솔리드 실린더 몸통
        BuildSolidCylinder(list, Vector3::Zero(), axis, AXIS_BODY_RADIUS, BODY_LENGTH, color, AXIS_SEGMENTS);

        // 솔리드 원뿔 머리
        const Vector3 cone_base = axis * BODY_LENGTH;
        BuildSolidCone(list, cone_base, axis, TRANSLATE_HEAD_RADIUS, TRANSLATE_HEAD_LENGTH, color, AXIS_SEGMENTS);
    }

    // 기준점 구체
    list.SetPickId(EncodePickID(EGizmoAxis::All));
    BuildSolidSphere(list, Vector3::Zero(), ORIGIN_SPHERE_RADIUS, GetAxisColor(EGizmoAxis::All), SPHERE_RINGS, SPHERE_SECTORS);

    // XY/XZ/YZ 평면 핸들 (채운 쿼드)
    struct PlaneInfo { i32 a0; i32 a1; EGizmoAxis axis; };
    constexpr PlaneInfo PLANE_INFOS[3] = {
        { 0, 1, EGizmoAxis::XY },
        { 0, 2, EGizmoAxis::XZ },
        { 1, 2, EGizmoAxis::YZ },
    };

    constexpr f64 OFFSET = PLANE_HANDLE_OFFSET;
    constexpr f64 LENGTH = PLANE_HANDLE_LENGTH;

    for (const auto& [a0, a1, plane_axis] : PLANE_INFOS)
    {
        const LinearColor color = GetAxisColor(plane_axis);

        const Vector3 corner = axes[a0] * OFFSET + axes[a1] * OFFSET;
        const Vector3 edge0  = axes[a0] * (OFFSET - LENGTH) + axes[a1] * OFFSET;
        const Vector3 edge1  = axes[a0] * OFFSET + axes[a1] * (OFFSET - LENGTH);
        const Vector3 inner  = axes[a0] * (OFFSET - LENGTH) + axes[a1] * (OFFSET - LENGTH);

        list.SetPickId(EncodePickID(plane_axis));
        list.AddTriangle(
            { .position = static_cast<Vector3f>(inner),  .color = color },
            { .position = static_cast<Vector3f>(edge0),  .color = color },
            { .position = static_cast<Vector3f>(corner), .color = color }
        );
        list.AddTriangle(
            { .position = static_cast<Vector3f>(inner),  .color = color },
            { .position = static_cast<Vector3f>(corner), .color = color },
            { .position = static_cast<Vector3f>(edge1),  .color = color }
        );
    }
}

void GizmoRenderer::DrawRotate(GizmoDrawList& list, const Quaternion& rot, const Vector3& direction_to_widget)
{
    // 3축: X(Right), Y(Forward), Z(Up)
    // 각 축의 ring은 나머지 두 축(Axis0, Axis1)이 이루는 평면에 놓임
    // Axis0 = ring의 0도 방향, Axis1 = ring의 90도 방향
    constexpr usize AXIS_0_IDX[3] = { 1, 2, 0 }; // X ring -> Axis0=Y, Y ring -> Axis0=Z, Z ring -> Axis0=X
    constexpr usize AXIS_1_IDX[3] = { 2, 0, 1 }; // X ring -> Axis1=Z, Y ring -> Axis1=X, Z ring -> Axis1=Y
    constexpr EGizmoAxis AXIS_IDS[3] = { EGizmoAxis::X, EGizmoAxis::Y, EGizmoAxis::Z };

    const Vector3 axes[3] = {
        rot.GetRightVector(),
        rot.GetForwardVector(),
        rot.GetUpVector()
    };

    for (usize i = 0; i < 3; ++i)
    {
        const LinearColor color = GetAxisColor(AXIS_IDS[i]);
        const Vector3& axis0 = axes[AXIS_0_IDX[i]];
        const Vector3& axis1 = axes[AXIS_1_IDX[i]];

        // 카메라 방향과의 내적으로 축을 반전하여 카메라 쪽 사분면 선택
        const bool mirror_axis0 = (axis0.Dot(direction_to_widget) <= 0.0);
        const bool mirror_axis1 = (axis1.Dot(direction_to_widget) <= 0.0);
        const Vector3 render_axis0 = mirror_axis0 ? axis0 : -axis0;
        const Vector3 render_axis1 = mirror_axis1 ? axis1 : -axis1;

        // Pick ID 설정
        list.SetPickId(EncodePickID(AXIS_IDS[i]));

        BuildThickArc(
            list, Vector3::Zero(),
            render_axis0, render_axis1,
            0.0_rad, Radian{ PI_DOUBLE / 2.0 },
            RING_INNER_RADIUS, RING_OUTER_RADIUS,
            RING_SEGMENTS, color
        );
    }
}

void GizmoRenderer::DrawScale(GizmoDrawList& list, const Quaternion& rot)
{
    const Vector3 axes[3] = {
        rot.GetRightVector(),
        rot.GetForwardVector(),
        rot.GetUpVector()
    };
    constexpr EGizmoAxis AXIS_IDS[3] = { EGizmoAxis::X, EGizmoAxis::Y, EGizmoAxis::Z };

    for (usize i = 0; i < 3; ++i)
    {
        const Vector3& axis = axes[i];
        const LinearColor color = GetAxisColor(AXIS_IDS[i]);

        // Pick ID 설정
        list.SetPickId(EncodePickID(AXIS_IDS[i]));

        // 솔리드 실린더 몸통
        BuildSolidCylinder(list, Vector3::Zero(), axis, AXIS_BODY_RADIUS, AXIS_LENGTH, color, AXIS_SEGMENTS);

        // 끝점 큐브
        const Vector3 endpoint = axis * AXIS_LENGTH;
        BuildSolidCube(list, endpoint, SCALE_CUBE_HALF, axes[0], axes[2], axes[1], color);
    }

    // 중앙 큐브 (전체 균등 스케일)
    const LinearColor center_color = GetAxisColor(EGizmoAxis::All);
    list.SetPickId(EncodePickID(EGizmoAxis::All));
    BuildSolidCube(list, Vector3::Zero(), SCALE_CUBE_HALF * 0.8, axes[0], axes[2], axes[1], center_color);

    // XY/XZ/YZ 평면 핸들 (채운 쿼드)
    struct PlaneInfo { i32 a0; i32 a1; EGizmoAxis axis; };
    constexpr PlaneInfo PLANES[3] = {
        { 0, 1, EGizmoAxis::XY },
        { 0, 2, EGizmoAxis::XZ },
        { 1, 2, EGizmoAxis::YZ },
    };

    constexpr f64 OFFSET = PLANE_HANDLE_OFFSET;
    constexpr f64 LENGTH = PLANE_HANDLE_LENGTH;

    for (const auto& [a0, a1, plane_axis] : PLANES)
    {
        const LinearColor color = GetAxisColor(plane_axis);

        const Vector3 corner = axes[a0] * OFFSET + axes[a1] * OFFSET;
        const Vector3 edge0  = axes[a0] * (OFFSET - LENGTH) + axes[a1] * OFFSET;
        const Vector3 edge1  = axes[a0] * OFFSET + axes[a1] * (OFFSET - LENGTH);
        const Vector3 inner  = axes[a0] * (OFFSET - LENGTH) + axes[a1] * (OFFSET - LENGTH);

        list.SetPickId(EncodePickID(plane_axis));
        list.AddTriangle(
            { .position = static_cast<Vector3f>(inner),  .color = color },
            { .position = static_cast<Vector3f>(edge0),  .color = color },
            { .position = static_cast<Vector3f>(corner), .color = color }
        );
        list.AddTriangle(
            { .position = static_cast<Vector3f>(inner),  .color = color },
            { .position = static_cast<Vector3f>(corner), .color = color },
            { .position = static_cast<Vector3f>(edge1),  .color = color }
        );
    }
}

LinearColor GizmoRenderer::GetAxisColor(EGizmoAxis axis) const
{
    if (axis == highlight_axis) { return LinearColor::Yellow(); }

    // NOLINTBEGIN(*-branch-clone)
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
    // NOLINTEND(*-branch-clone)
}

void GizmoRenderer::BuildSolidCylinder(
    GizmoDrawList& list, const Vector3& base_center, const Vector3& axis_dir,
    f64 radius, f64 height, const LinearColor& color, i32 segments
)
{
    Vector3 tangent, bitangent;
    axis_dir.GetOrthogonalAxes(tangent, bitangent);

    const Vector3 top_center = base_center + axis_dir * height;
    const Radian angle_step = Radian{ PI_DOUBLE * 2.0 } / segments;

    for (i32 i = 0; i < segments; ++i)
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
            { .position = static_cast<Vector3f>(b0), .color = color },
            { .position = static_cast<Vector3f>(b1), .color = color },
            { .position = static_cast<Vector3f>(t0), .color = color }
        );
        list.AddTriangle(
            { .position = static_cast<Vector3f>(b1), .color = color },
            { .position = static_cast<Vector3f>(t1), .color = color },
            { .position = static_cast<Vector3f>(t0), .color = color }
        );
    }
}

void GizmoRenderer::BuildSolidCone(
    GizmoDrawList& list, const Vector3& base_center, const Vector3& axis_dir,
    f64 radius, f64 height, const LinearColor& color, i32 segments
)
{
    Vector3 tangent, bitangent;
    axis_dir.GetOrthogonalAxes(tangent, bitangent);

    const Vector3 tip = base_center + axis_dir * height;
    const Radian angle_step = Radian{ PI_DOUBLE * 2.0 } / segments;

    for (i32 i = 0; i < segments; ++i)
    {
        const Radian a0 = angle_step * i;
        const Radian a1 = angle_step * (i + 1);

        const Vector3 p0 = base_center + (tangent * Cos(a0) + bitangent * Sin(a0)) * radius;
        const Vector3 p1 = base_center + (tangent * Cos(a1) + bitangent * Sin(a1)) * radius;

        list.AddTriangle(
            { .position = static_cast<Vector3f>(tip), .color = color },
            { .position = static_cast<Vector3f>(p0),  .color = color },
            { .position = static_cast<Vector3f>(p1),  .color = color }
        );
    }
}

void GizmoRenderer::BuildSolidCube(
    GizmoDrawList& list, const Vector3& center, f64 half_extent,
    const Vector3& right, const Vector3& up, const Vector3& forward,
    const LinearColor& color
)
{
    // 큐브를 구성하는 8개의 로컬 꼭짓점 계산
    const Vector3 corners[8] = {
        center + (-right - forward - up) * half_extent, // 0: 좌하단 후면
        center + ( right - forward - up) * half_extent, // 1: 우하단 후면
        center + ( right + forward - up) * half_extent, // 2: 우하단 전면
        center + (-right + forward - up) * half_extent, // 3: 좌하단 전면
        center + (-right - forward + up) * half_extent, // 4: 좌상단 후면
        center + ( right - forward + up) * half_extent, // 5: 우상단 후면
        center + ( right + forward + up) * half_extent, // 6: 우상단 전면
        center + (-right + forward + up) * half_extent, // 7: 좌상단 전면
    };

    // 6개의 면 정의 (반시계 방향으로 꼭짓점 인덱스 정렬)
    struct Face { i32 i0, i1, i2, i3; };
    const Face faces[6] = {
        { 1, 2, 6, 5 }, // +X
        { 3, 0, 4, 7 }, // -X
        { 2, 3, 7, 6 }, // +Y
        { 0, 1, 5, 4 }, // -Y
        { 4, 5, 6, 7 }, // +Z
        { 0, 3, 2, 1 }, // -Z
    };

    for (const Face& face : faces)
    {
        // 한 면을 두 개의 삼각형으로 분할
        list.AddTriangle(
            { .position = static_cast<Vector3f>(corners[face.i0]), .color = color },
            { .position = static_cast<Vector3f>(corners[face.i1]), .color = color },
            { .position = static_cast<Vector3f>(corners[face.i2]), .color = color }
        );
        list.AddTriangle(
            { .position = static_cast<Vector3f>(corners[face.i0]), .color = color },
            { .position = static_cast<Vector3f>(corners[face.i2]), .color = color },
            { .position = static_cast<Vector3f>(corners[face.i3]), .color = color }
        );
    }
}

void GizmoRenderer::BuildSolidSphere(GizmoDrawList& list, const Vector3& center, f64 radius, const LinearColor& color, i32 rings, i32 sectors)
{
    // UV 구체 정점 위치 계산 (Z-up)
    auto get_point = [&](i32 ring, i32 sector) -> Vector3
    {
        const Radian phi = Radian{ PI_DOUBLE } * ring / rings;
        const Radian theta = Radian{ PI_DOUBLE * 2.0 } * sector / sectors;

        return center + Vector3{
            Sin(phi) * Cos(theta) * radius,
            Sin(phi) * Sin(theta) * radius,
            Cos(phi) * radius
        };
    };

    for (i32 i = 0; i < rings; ++i)
    {
        for (i32 j = 0; j < sectors; ++j)
        {
            const i32 j_next = (j + 1) % sectors;

            const Vector3 p00 = get_point(i, j);
            const Vector3 p10 = get_point(i + 1, j);
            const Vector3 p01 = get_point(i, j_next);
            const Vector3 p11 = get_point(i + 1, j_next);

            if (i == 0)
            {
                // 상단 극점: 삼각형 팬
                list.AddTriangle(
                    { .position = static_cast<Vector3f>(p00), .color = color },
                    { .position = static_cast<Vector3f>(p10), .color = color },
                    { .position = static_cast<Vector3f>(p11), .color = color }
                );
            }
            else if (i == rings - 1)
            {
                // 하단 극점: 삼각형 팬
                list.AddTriangle(
                    { .position = static_cast<Vector3f>(p00), .color = color },
                    { .position = static_cast<Vector3f>(p01), .color = color },
                    { .position = static_cast<Vector3f>(p10), .color = color }
                );
            }
            else
            {
                // 일반 쿼드: 두 삼각형
                list.AddTriangle(
                    { .position = static_cast<Vector3f>(p00), .color = color },
                    { .position = static_cast<Vector3f>(p10), .color = color },
                    { .position = static_cast<Vector3f>(p11), .color = color }
                );
                list.AddTriangle(
                    { .position = static_cast<Vector3f>(p00), .color = color },
                    { .position = static_cast<Vector3f>(p11), .color = color },
                    { .position = static_cast<Vector3f>(p01), .color = color }
                );
            }
        }
    }
}

void GizmoRenderer::BuildThickArc(
    GizmoDrawList& list, const Vector3& center,
    const Vector3& axis0, const Vector3& axis1,
    Radian<f64> start_angle, Radian<f64> end_angle,
    f64 inner_radius, f64 outer_radius,
    i32 segments, const LinearColor& color
)
{
    const Radian angle_step = (end_angle - start_angle) / segments;

    for (i32 i = 0; i < segments; ++i)
    {
        const Radian a0 = start_angle + angle_step * i;
        const Radian a1 = start_angle + angle_step * (i + 1);

        const Vector3 dir0 = axis0 * Cos(a0) + axis1 * Sin(a0);
        const Vector3 dir1 = axis0 * Cos(a1) + axis1 * Sin(a1);

        const Vector3 inner0 = center + dir0 * inner_radius;
        const Vector3 inner1 = center + dir1 * inner_radius;
        const Vector3 outer0 = center + dir0 * outer_radius;
        const Vector3 outer1 = center + dir1 * outer_radius;

        // 내부 -> 외부 쿼드 (2 삼각형)
        list.AddTriangle(
            { .position = static_cast<Vector3f>(inner0), .color = color },
            { .position = static_cast<Vector3f>(outer0), .color = color },
            { .position = static_cast<Vector3f>(outer1), .color = color }
        );
        list.AddTriangle(
            { .position = static_cast<Vector3f>(inner0), .color = color },
            { .position = static_cast<Vector3f>(outer1), .color = color },
            { .position = static_cast<Vector3f>(inner1), .color = color }
        );
    }
}
} // namespace se::editor

// NOLINTEND(*-use-designated-initializers, *-isolate-declaration)
