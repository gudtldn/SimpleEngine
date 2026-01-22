#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Reflection/Annotations.h"


namespace se
{
/**
 * 3D 카메라의 렌즈 특성(시야각, 클리핑 평면)을 정의하는 컴포넌트
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=meta::Component) Camera3dComponent
{
    SE_PROPERTY(=meta::Edit)
    Degree<double> fov = 90.0_deg;

    SE_PROPERTY(=meta::Edit)
    double near_plane = 0.1;

    SE_PROPERTY(=meta::Edit)
    double far_plane = 10'000.0;
};
}  // namespace se
