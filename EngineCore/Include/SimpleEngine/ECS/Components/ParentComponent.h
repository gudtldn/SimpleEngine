#pragma once
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Reflection/Annotations.h"


/**
 * 현재 Entity의 부모 Entity를 지정합니다.
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=se::meta::Component) ParentComponent
{
public:
    SE_PROPERTY(=se::meta::Edit)
    se::world::Entity parent;
};
