#pragma once
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * 3D 카메라의 렌즈 특성(시야각, 클리핑 평면)을 정의하는 컴포넌트
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) Camera3dComponent
{
    SE_ANNOTATION(=meta::Property)
    Degree<double> fov = 90.0_deg;

    SE_ANNOTATION(=meta::Property)
    double near_plane = 0.1;

    SE_ANNOTATION(=meta::Property)
    double far_plane = 10'000.0;
};
}  // namespace se

SE_DECLARE_REFLECTION(se::Camera3dComponent)
