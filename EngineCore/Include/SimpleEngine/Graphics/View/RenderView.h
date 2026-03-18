#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se::graphics
{
/**
 * 단일 뷰포트의 렌더링에 필요한 카메라/타겟 정보를 담는 구조체
 */
struct RenderView
{
    Matrix4x4 view_matrix = Matrix4x4::Identity();
    Matrix4x4 projection_matrix = Matrix4x4::Identity();

    StringName color_target_name;
    StringName depth_target_name;

    uint32 width = 0;
    uint32 height = 0;
};
} // namespace se::graphics
