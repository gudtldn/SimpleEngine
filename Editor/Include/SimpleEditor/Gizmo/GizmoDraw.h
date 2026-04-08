#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEngine/Core/Math/Color.h"
#include "SimpleEngine/Core/Math/Math.h"


namespace se::editor
{
/**
 * 두 점을 잇는 단일 기즈모 라인을 그립니다.
 * @param start 라인 시작점 (월드 좌표)
 * @param end 라인 끝점 (월드 좌표)
 * @param color 라인 색상
 */
SE_EDITOR_API void DrawGizmoLine(
    const Vector3& start,
    const Vector3& end,
    const LinearColor& color = LinearColor::White()
);

/**
 * 세 점을 잇는 단색 기즈모 삼각형을 그립니다.
 * @param a 삼각형 첫 번째 꼭짓점 (월드 좌표)
 * @param b 삼각형 두 번째 꼭짓점 (월드 좌표)
 * @param c 삼각형 세 번째 꼭짓점 (월드 좌표)
 * @param color 삼각형 색상
 */
SE_EDITOR_API void DrawGizmoTriangle(
    const Vector3& a,
    const Vector3& b,
    const Vector3& c,
    const LinearColor& color = LinearColor::White()
);

/**
 * 라인과 원뿔 머리로 구성된 3D 화살표를 그립니다.
 * @param from 화살표 시작점 (월드 좌표)
 * @param to 화살표 끝점 (월드 좌표, 원뿔 꼭대기)
 * @param head_size 화살촉(원뿔) 밑면의 반지름
 * @param color 화살표 색상
 * @param segments 원뿔을 구성하는 해상도 (기본값 8)
 */
SE_EDITOR_API void DrawGizmoArrow(
    const Vector3& from,
    const Vector3& to,
    float head_size,
    const LinearColor& color = LinearColor::White(),
    int32 segments = 8
);

/**
 * 3D 공간 상의 원(Circle)을 라인 세그먼트로 근사하여 그립니다.
 * @param center 원 중심점 (월드 좌표)
 * @param normal 원이 놓이는 평면의 법선 벡터 (바라보는 방향)
 * @param radius 원의 반지름
 * @param color 원 색상
 * @param segments 원을 구성하는 라인 수 (높을수록 매끄러움)
 */
SE_EDITOR_API void DrawGizmoCircle(
    const Vector3& center,
    const Vector3& normal,
    float radius,
    const LinearColor& color = LinearColor::White(),
    int32 segments = 32
);

/**
 * 특정 위치에 월드 좌표계의 3축(X, Y, Z) 기즈모를 그립니다. (+X=Red, +Y=Green, +Z=Blue)
 */
SE_EDITOR_API void DrawGizmoWorldAxes(
    const Vector3& origin = Vector3::Zero(),
    float length = 1.0f
);
} // namespace se::editor
