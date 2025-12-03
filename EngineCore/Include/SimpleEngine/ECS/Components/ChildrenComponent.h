#pragma once
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Reflection/Annotations.h"


/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=se::meta::Component) ChildrenComponent
{
    SE_PROPERTY(=se::meta::Edit)
    se::Array<se::world::Entity> children;
};
