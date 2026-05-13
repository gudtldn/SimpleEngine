#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se::editor
{
/**
 * 에디터 뷰포트의 카메라 상태를 담는 구조체
 */
struct SE_ANNOTATION(=meta::EditorOnly) EditorCameraState
{
    SE_ANNOTATION(=meta::Property)
    Vector3 position = { 0.0, -5.0, 2.0 };

    SE_ANNOTATION(=meta::Property)
    Rotator rotation = Rotator::ZeroRotator();

    SE_ANNOTATION(=meta::Property)
    Vector3 velocity = Vector3::Zero();

    SE_ANNOTATION(=meta::Property, =meta::Range(0.1f, 10000.0f))
    f64 ortho_width = 100.0;

    SE_ANNOTATION(=meta::Property, =meta::Range(0.0f, 180.0f))
    Degree<f64> fov_y = 60.0_deg;

    SE_ANNOTATION(=meta::Property, =meta::Range(0.001f, 100.0f))
    f64 near_plane = 0.1;

    SE_ANNOTATION(=meta::Property, =meta::Range(1.0f, 100'000.0f))
    f64 far_plane = 10000.0;

    SE_ANNOTATION(=meta::Property, =meta::Range(0.01f, 1000.0f))
    f64 move_speed = 5.0;

    SE_ANNOTATION(=meta::Property, =meta::Range(0.001f, 10.0f))
    f64 look_sensitivity = 0.15;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::EditorCameraState);
