#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Gizmo/GizmoTypes.h"

#include "SimpleEngine/Core/Math/Math.h"

// forward declaration
namespace se::graphics { struct RenderView; }


namespace se::editor
{
/**
 * 기즈모 조작(Translation, Rotation, Scale)을 위한 마우스 피킹 및 3D 드래그 계산을 담당하는 클래스
 *
 * 상태 흐름:
 * - Idle --(LMB down + hovered_axis != None)--> Dragging
 * - Dragging --(LMB up)--> Idle
 */
class SE_EDITOR_API GizmoInteraction
{
public:
    /** 드래그 결과 (모드에 따라 해당 필드만 유효) */
    struct DragResult
    {
        Vector3 translation_delta = Vector3::Zero();
        Quaternion rotation_delta = Quaternion::Identity();
        Vector3 scale_delta = Vector3::Zero();

        // true: 로컬 회전 -> Q * delta (right-multiply, 로컬 프레임 해석)
        // false: 월드 회전 -> delta * Q (left-multiply, 월드 프레임 해석)
        bool is_local_rotation = false;
    };

    /**
     * 드래그 상태를 초기화하고, 교차 판정의 시작 기준점을 캡처합니다.
     * @param in_mode 현재 기즈모 모드 (Translate, Rotate, Scale)
     * @param in_axis hover 중인 축
     * @param in_local true면 Local 좌표계, false면 World 좌표계
     * @param in_cursor_pos 뷰포트 기준 커서 좌표 (px)
     * @param in_gizmo_center 기즈모의 월드 공간 원점
     * @param in_gizmo_rotation 기즈모의 월드 회전 (Local 좌표계일 때 엔티티 회전)
     * @param in_view 뷰포트 및 카메라 투영 행렬 정보
     */
    void BeginDrag(
        EGizmoMode in_mode,
        EGizmoAxis in_axis,
        bool in_local,
        const Vector2f& in_cursor_pos,
        const Vector3& in_gizmo_center,
        const Quaternion& in_gizmo_rotation,
        const graphics::RenderView& in_view
    );

    /**
     * 마우스 이동에 따른 기하학적 Delta 변환값을 계산합니다.
     * @param cursor_pos 현재 뷰포트 로컬 커서 좌표 (px)
     * @param view 현재 RenderView
     * @return 이전 프레임 대비 변화한 변환량 (DragResult)
     */
    [[nodiscard]] DragResult UpdateDrag(const Vector2f& cursor_pos, const graphics::RenderView& view);

    /** 드래그 상태를 해제하고 내부 파라미터를 초기화합니다. */
    void EndDrag();

    [[nodiscard]] bool IsDragging() const { return dragging; }
    [[nodiscard]] EGizmoAxis GetActiveAxis() const { return active_axis; }

private:
    /** 특정 축(X, Y, Z)에 대응하는 방향 벡터를 반환합니다. (기즈모 회전 상태 반영) */
    [[nodiscard]] Vector3 GetAxisDirection(EGizmoAxis axis) const;

    /** 특정 평면 핸들(XY, XZ, YZ)에 수직인 법선 벡터(Normal)를 반환합니다. (기즈모 회전 상태 반영) */
    [[nodiscard]] Vector3 GetPlaneNormal(EGizmoAxis axis) const;

    [[nodiscard]] DragResult UpdateTranslation(const Ray& ray);
    [[nodiscard]] DragResult UpdateScale(const Ray& ray, const Vector2f& cursor_pos);
    [[nodiscard]] DragResult UpdateRotation(const Vector2f& cursor_pos, const graphics::RenderView& view);

private:
    bool dragging = false;
    bool is_local = false;  // Local 좌표계 여부 (Rotation 적용 순서 결정)
    EGizmoMode active_mode = EGizmoMode::Translate;
    EGizmoAxis active_axis = EGizmoAxis::None;

    // 조작 중인 기즈모의 월드 Transform 기준
    Vector3 drag_gizmo_center = Vector3::Zero();
    Quaternion drag_gizmo_rotation = Quaternion::Identity();

    // Translation & Scale 상태: 마우스 레이와 축의 최근접 교차점 파라미터(t)
    double drag_start_t = 0.0;

    // Rotation 상태: 평면 상에서의 시작 정보
    Radian<double> drag_start_angle = 0.0_rad;
    Vector3 drag_plane_normal = Vector3::Up();
    Vector3 drag_start_vector = Vector3::Right(); // 평면 위에서 드래그를 시작한 위치를 향하는 기준 벡터 (각도 계산용)
};
} // namespace se::editor
