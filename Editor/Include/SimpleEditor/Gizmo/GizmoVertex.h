#pragma once

#include "SimpleEngine/Core/Math/Math.h"


namespace se::editor
{
/**
 * 기즈모 렌더링에 사용되는 GPU 정점 데이터
 */
struct GizmoVertex
{
    Vector3f position; // 기즈모 로컬 공간 위치 (Center 기준 좌표)
    LinearColor color; // Linear 색상 (sRGB 아님)
};
} // namespace se::editor
