#pragma once
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se
{
/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=::se::meta::Component) ChildrenComponent
{
    SE_PROPERTY(=::se::meta::Edit)
    Array<Entity> children;
};
}  // namespace se
