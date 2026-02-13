#pragma once
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se
{
/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) ChildrenComponent
{
    SE_ANNOTATION(=meta::Property)
    Array<Entity> children;
};
}  // namespace se
