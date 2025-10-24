#pragma once
#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/World/Entity.h"


/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
struct SE_CORE_API ChildrenComponent
{
    se::vector<se::world::Entity> children;
};
