#pragma once

#include "SimpleEngine/Core/Math/Math.h"


namespace se::editor
{
/**
 * 기즈모 렌더링에 사용되는 GPU 정점 데이터
 */
struct GizmoVertex
{
    Vector3f position;  // 기즈모 로컬 공간 위치 (Center 기준 좌표)
    LinearColor color;  // Linear 색상 (sRGB 아님)
    uint32 pick_id = 0; // Pick ID
};

/**
 * 기즈모 셰이더 Uniform Buffer Object (cbuffer GizmoUBO : register(b0, space1) 대응)
 * GizmoPass와 GizmoPickPass에서 공유합니다.
 */
struct GizmoUBO
{
    Matrix4x4f vp;
    Vector3f gizmo_center;
    float screen_scale;
};
} // namespace se::editor
