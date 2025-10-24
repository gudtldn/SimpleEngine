#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"


/**
 * 3D 공간에서 Entity의 위치, 회전, 크기를 정의하는 컴포넌트
 */
struct SE_CORE_API TransformComponent
{
    Quaternion rotation = Quaternion::Identity();
    Vector3 position = Vector3::Zero();
    Vector3 scale = Vector3::One();
};
