#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Reflection/Reflect.h"


/**
 * 3D 카메라의 렌즈 특성(시야각, 클리핑 평면)을 정의하는 컴포넌트
 */
struct SE_CORE_API Camera3dComponent
{
    SE_REFLECTABLE()

public:
    SE_PROPERTY()
    Degree<float> fov = 90.0_degf;

    SE_PROPERTY()
    float near_plane = 0.1f;

    SE_PROPERTY()
    float far_plane = 10'000.0f;
};
