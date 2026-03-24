#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
/**
 * 에디터 뷰포트의 카메라 상태를 담는 구조체
 */
struct EditorCameraState
{
    Vector3 position = { 0.0, -5.0, 2.0 };
    Rotator rotation = Rotator::ZeroRotator();

    Vector3 velocity = Vector3::Zero();

    double move_speed = 5.0;
    double look_sensitivity = 0.15;
    Degree<double> fov_y = 60.0_deg;
    double near_plane = 0.1;
    double far_plane = 1000.0;

    /** 현재 카메라 상태로부터 RenderView를 계산합니다. */
    [[nodiscard]] graphics::RenderView ComputeRenderView(uint32 width, uint32 height) const;
};
} // namespace se::editor
