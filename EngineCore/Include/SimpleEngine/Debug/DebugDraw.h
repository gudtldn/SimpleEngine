#pragma once

#include "SimpleEngine/Core/Math/Color.h"
#include "SimpleEngine/Core/Math/Math.h"


namespace se
{
// forward declaration
struct DebugLine;

/**
 * 월드 공간의 두 점을 직선으로 그립니다.
 * @param start 시작점 (월드 좌표)
 * @param end 끝점 (월드 좌표)
 * @param color 선 색상 (Linear)
 * @param duration 유지 시간(초). 0 = 이번 프레임만. (0 초과: 추후 지원 예정)
 */
SE_CORE_API void DrawDebugLine(
    const Vector3& start,
    const Vector3& end,
    const LinearColor& color = LinearColor::White(),
    float duration = 0.0f
);

/**
 * 여러 개의 DebugLine을 한 번에 그립니다.
 * @param lines 그려질 DebugLine 데이터들의 ArrayView (DebugLine 구조체 집합)
 */
SE_CORE_API void DrawDebugLines(ArrayView<const DebugLine> lines);

/** 원점에서 방향으로 선을 그립니다. */
SE_CORE_API void DrawDebugRay(
    const Ray& ray,
    double length,
    const LinearColor& color = LinearColor::White(),
    float duration = 0.0f
);

/**
 * 월드 좌표계 기준축을 그립니다.
 * +X(Right)=빨강, +Y(Forward)=초록, +Z(Up)=파랑
 */
SE_CORE_API void DrawDebugWorldAxes(
    const Vector3& origin = Vector3::Zero(),
    float length = 1000.0f
);
} // namespace se
