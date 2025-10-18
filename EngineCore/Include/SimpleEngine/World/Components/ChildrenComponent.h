#pragma once
#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Reflection/Reflect.h"
#include "SimpleEngine/World/Entity.h"


/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
struct SE_CORE_API ChildrenComponent
{
    SE_REFLECTABLE()

public:
    SE_PROPERTY()
    se::vector<se::world::Entity> children;
};
