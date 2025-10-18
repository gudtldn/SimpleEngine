#pragma once
#include "SimpleEngine/Reflection/Reflect.h"
#include "SimpleEngine/World/World.h"


/**
 * 현재 Entity의 부모 Entity를 지정합니다.
 */
struct SE_CORE_API ParentComponent
{
    SE_REFLECTABLE()

public:
    SE_PROPERTY()
    se::world::Entity parent;
};
