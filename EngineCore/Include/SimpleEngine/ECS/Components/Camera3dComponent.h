#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"


namespace se
{
/**
 * 3D 카메라의 렌즈 특성(시야각, 클리핑 평면)을 정의하는 컴포넌트
 */
struct SE_CORE_API Camera3dComponent
{
    Degree<f64> fov = 90.0_deg;

    f64 near_plane = 0.1;

    f64 far_plane = 10'000.0;
};
} // namespace se

SE_DECLARE_REFLECTION(se::Camera3dComponent, SE_CORE_API)
