#pragma once

#include "../../Core/Reflection/Legacy/Annotations.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se
{
/**
 * 현재 Entity의 부모 Entity를 지정합니다.
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) ParentComponent
{
public:
    SE_ANNOTATION(=meta::Reflect)
    Entity parent;
};
} // namespace se

SE_DECLARE_REFLECTION_V1(se::ParentComponent)
