#pragma once
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Reflection/Annotations.h"


namespace se
{
/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=meta::Component) ChildrenComponent
{
    SE_PROPERTY(=meta::Edit)
    Array<Entity> children;
};
}
