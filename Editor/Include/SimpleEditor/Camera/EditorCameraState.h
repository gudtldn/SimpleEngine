#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "../../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Annotations.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"


namespace se::editor
{
/**
 * 에디터 뷰포트의 카메라 상태를 담는 구조체
 */
struct SE_ANNOTATION(=meta::Reflect, =meta::Transient) EditorCameraState
{
    SE_ANNOTATION(=meta::Reflect)
    Vector3 position = { 0.0, -5.0, 2.0 };

    SE_ANNOTATION(=meta::Reflect)
    Rotator rotation = Rotator::ZeroRotator();

    SE_ANNOTATION(=meta::Reflect)
    Vector3 velocity = Vector3::Zero();

    SE_ANNOTATION(=meta::Reflect, =meta::Range(0.1f, 10000.0f))
    SE_ANNOTATE(ortho_width, display::Range(0.1f, 10000.0f))
    f64 ortho_width = 100.0;

    SE_ANNOTATION(=meta::Reflect, =meta::Range(0.0f, 180.0f))
    SE_ANNOTATE(fov_y, display::Range(0.0f, 180.0f))
    Degree<f64> fov_y = 60.0_deg;

    SE_ANNOTATION(=meta::Reflect, =meta::Range(0.001f, 100.0f))
    SE_ANNOTATE(near_plane, display::Range(0.001f, 100.0f))
    f64 near_plane = 0.1;

    SE_ANNOTATION(=meta::Reflect, =meta::Range(1.0f, 100'000.0f))
    SE_ANNOTATE(far_plane, display::Range(1.0f, 100'000.0f))
    f64 far_plane = 10000.0;

    SE_ANNOTATION(=meta::Reflect, =meta::Range(0.01f, 1000.0f))
    SE_ANNOTATE(move_speed, display::Range(0.01f, 1000.0f))
    f64 move_speed = 5.0;

    SE_ANNOTATION(=meta::Reflect, =meta::Range(0.001f, 10.0f))
    SE_ANNOTATE(look_sensitivity, display::Range(0.001f, 10.0f))
    f64 look_sensitivity = 0.15;
};
} // namespace se::editor

SE_DECLARE_REFLECTION_V1(se::editor::EditorCameraState);
SE_DECLARE_REFLECTION(se::editor::EditorCameraState, SE_EDITOR_API)
