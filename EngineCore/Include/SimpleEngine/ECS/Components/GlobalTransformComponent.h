#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * Entity의 최종 월드 공간 변환 행렬입니다.
 * TransformComponent(로컬)로부터 계층 구조를 따라 계산됩니다.
 */
struct SE_CORE_API SE_ANNOTATION(=meta::EditorOnly, =meta::Component) GlobalTransformComponent
{
    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    Matrix4x4 value = Matrix4x4::Identity();
};
} // namespace se

SE_DECLARE_REFLECTION(se::GlobalTransformComponent)
