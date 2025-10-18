#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Reflection/Reflect.h"


/**
 * 3D 공간에서 Entity의 위치, 회전, 크기를 정의하는 컴포넌트
 */
struct SE_CORE_API TransformComponent
{
    SE_REFLECTABLE()

public:
    SE_PROPERTY()
    Quaternion rotation = Quaternion::Identity();

    SE_PROPERTY()
    Vector3 position = Vector3::Zero();

    SE_PROPERTY()
    Vector3 scale = Vector3::One();
};
