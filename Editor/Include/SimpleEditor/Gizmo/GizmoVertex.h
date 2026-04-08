#pragma once

#include "SimpleEngine/Core/Math/Math.h"


namespace se::editor
{
/**
 * 기즈모 렌더링에 사용되는 GPU 정점 데이터
 */
struct GizmoVertex
{
    Vector3f position;  // 월드 공간 위치
    LinearColor color;  // Linear 색상 (sRGB 아님)
};
} // namespace se::editor
