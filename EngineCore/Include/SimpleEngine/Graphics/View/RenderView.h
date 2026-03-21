#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"


namespace se::graphics
{
/**
 * 단일 뷰포트의 렌더링에 필요한 카메라 정보를 담는 순수 POD 구조체
 */
struct RenderView
{
    Matrix4x4 view_matrix = Matrix4x4::Identity();
    Matrix4x4 projection_matrix = Matrix4x4::Identity();

    uint32 width = 0;
    uint32 height = 0;

    float near_plane = 0.1f;
    float far_plane = 1000.0f;
};
} // namespace se::graphics
