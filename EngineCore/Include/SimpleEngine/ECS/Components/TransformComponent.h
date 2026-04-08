#pragma once
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * 3D 공간에서 Entity의 위치, 회전, 크기를 정의하는 컴포넌트
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) TransformComponent
{
    SE_ANNOTATION(=meta::Property)
    Quaternion rotation = Quaternion::Identity();

    SE_ANNOTATION(=meta::Property)
    Vector3 position = Vector3::Zero();

    SE_ANNOTATION(=meta::Property)
    Vector3 scale = Vector3::One();

    // PropagateTransforms에서 최적화용 flag로 사용
    bool dirty = true;
};
}  // namespace se

SE_DECLARE_REFLECTION(se::TransformComponent)
