#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Meta/Annotations.h"


namespace se
{
/**
 * 3D 공간에서 Entity의 위치, 회전, 크기를 정의하는 컴포넌트
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=::se::meta::Component) TransformComponent
{
    SE_PROPERTY(=::se::meta::Edit)
    Quaternion rotation = Quaternion::Identity();

    SE_PROPERTY(=::se::meta::Edit)
    Vector3 position = Vector3::Zero();

    SE_PROPERTY(=::se::meta::Edit)
    Vector3 scale = Vector3::One();
};
}  // namespace se
