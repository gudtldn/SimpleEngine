#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Gizmo/GizmoTypes.h"
#include "SimpleEngine/Core/Math/Color.h"
#include "SimpleEngine/Core/Math/Math.h"


namespace se::graphics { struct RenderView; }

namespace se::editor
{
class GizmoDrawList;

/**
 * 선택된 엔티티의 Transform(Translate/Rotate/Scale)을 제어하는 3D 기즈모를 절차적으로 생성하는 클래스입니다.
 */
class SE_EDITOR_API GizmoRenderer
{
public:
    /** 기즈모 모드를 설정합니다. */
    void SetMode(EGizmoMode in_mode);
    [[nodiscard]] EGizmoMode GetMode() const { return mode; }

    /**
     * 기즈모의 하이라이트 축을 설정합니다.
     * @param axis 마우스 호버 시 강조할 축
     */
    void SetHighlightAxis(EGizmoAxis axis);
    [[nodiscard]] EGizmoAxis GetHighlightAxis() const { return highlight_axis; }

    /**
     * 현재 설정된 모드의 기즈모 형상을 조립하여 DrawList에 제출합니다.
     * @param draw_list 프리미티브를 제출할 드로우 리스트
     * @param rotation 대상의 World Space 회전
     */
    void Draw(GizmoDrawList& draw_list, const Quaternion& rotation);

    /**
     * 기즈모가 화면에서 일정한 픽셀 크기를 유지하도록 원근 보정용 Scale 값을 계산합니다.
     * @param position 기즈모 월드 위치
     * @param view 카메라 투영 정보
     * @return 기즈모에 적용할 월드 공간 스케일
     */
    [[nodiscard]] static double ComputeScreenScale(const Vector3& position, const graphics::RenderView& view);

private:
    void DrawTranslate(GizmoDrawList& list, const Quaternion& rot);
    void DrawRotate(GizmoDrawList& list, const Quaternion& rot);
    void DrawScale(GizmoDrawList& list, const Quaternion& rot);

    /**
     * 하이라이트 상태를 고려하여 축의 색상을 반환합니다.
     * @param axis 대상 축
     * @return 하이라이트 시 Yellow, 아니면 축별 기본색 (X=Red, Y=Green, Z=Blue)
     */
    [[nodiscard]] LinearColor GetAxisColor(EGizmoAxis axis) const;

private:
    /**
     * 솔리드 실린더를 삼각형으로 조립합니다. (Translate 화살표 몸통용)
     * @param base_center 실린더 밑면 중심 (월드 좌표)
     * @param axis_dir 실린더 축 방향 (정규화 필요)
     * @param radius 실린더 반지름
     * @param height 실린더 높이
     * @param color 기본 색상
     * @param segments 원 분할 수
     */
    static void BuildSolidCylinder(
        GizmoDrawList& list, const Vector3& base_center, const Vector3& axis_dir,
        double radius, double height, const LinearColor& color, int32 segments
    );

    /**
     * 솔리드 원뿔을 삼각형 팬으로 조립합니다. (Translate 화살표 머리용)
     * @param base_center 원뿔 밑면 중심 (월드 좌표)
     * @param axis_dir 원뿔 축 방향 (정규화 필요)
     * @param radius 원뿔 밑면 반지름
     * @param height 원뿔 높이
     * @param color 기본 색상
     * @param segments 원 분할 수
     */
    static void BuildSolidCone(
        GizmoDrawList& list, const Vector3& base_center, const Vector3& axis_dir,
        double radius, double height, const LinearColor& color, int32 segments
    );

    /**
     * 솔리드 큐브를 12개 삼각형(6면)으로 조립합니다. (Scale 끝점 핸들용)
     * @param center 큐브 중심 (월드 좌표)
     * @param half_extent 큐브 반변 길이
     * @param right 큐브 로컬 X축
     * @param up 큐브 로컬 Z축
     * @param forward 큐브 로컬 Y축
     * @param color 기본 색상
     */
    static void BuildSolidCube(
        GizmoDrawList& list, const Vector3& center, double half_extent,
        const Vector3& right, const Vector3& up, const Vector3& forward,
        const LinearColor& color
    );

    /**
     * 솔리드 UV 구체를 삼각형으로 조립합니다. (Translate 기준점 구체용)
     * @param center 구체 중심 (월드 좌표)
     * @param radius 구체 반지름
     * @param color 기본 색상
     * @param rings 위도 분할 수
     * @param sectors 경도 분할 수
     */
    static void BuildSolidSphere(
        GizmoDrawList& list, const Vector3& center,
        double radius, const LinearColor& color,
        int32 rings, int32 sectors
    );

private:
    EGizmoMode mode = EGizmoMode::Translate;
    EGizmoAxis highlight_axis = EGizmoAxis::None;

    // TODO: 아래 내용 설정파일이나, 에디터에서 조작할 수 있도록 수정

    // 기즈모가 화면 높이 대비 차지하는 비율 (FOV 무관하게 일정한 크기를 위함)
    static constexpr double GIZMO_SCREEN_RATIO = 0.28f;

    // 공통 축 파라미터 (Translate/Scale 몸통 실린더 공유)
    static constexpr double AXIS_LENGTH = 1.0f;
    static constexpr double AXIS_BODY_RADIUS = 0.025f;
    static constexpr int32 AXIS_SEGMENTS = 12;

    // Translate 모드
    static constexpr double TRANSLATE_HEAD_RADIUS = 0.07f;
    static constexpr double TRANSLATE_HEAD_LENGTH = 0.2f;
    static constexpr double ORIGIN_SPHERE_RADIUS = 0.06f;
    static constexpr int32 SPHERE_RINGS = 6;
    static constexpr int32 SPHERE_SECTORS = 8;

    // Rotate 모드
    static constexpr double CIRCLE_RADIUS = 1.0f;
    static constexpr int32 CIRCLE_SEGMENTS = 64;

    // Scale 모드
    static constexpr double SCALE_CUBE_HALF = 0.05f;

    // 평면 핸들 (XY/XZ/YZ 꺾쇠)
    static constexpr double PLANE_HANDLE_OFFSET = 0.35f;
    static constexpr double PLANE_HANDLE_LENGTH = 0.15f;
};
} // namespace se::editor
