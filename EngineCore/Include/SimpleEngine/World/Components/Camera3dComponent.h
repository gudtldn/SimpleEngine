#pragma once
#include "SimpleEngine/Core/Math/Math.h"


/**
 * 3D 카메라의 렌즈 특성(시야각, 클리핑 평면)을 정의하는 컴포넌트
 */
struct SE_CORE_API Camera3dComponent
{
public:
    Degree<double> fov = 90.0_deg;
    double near_plane = 0.1f;
    double far_plane = 10'000.0f;
};
