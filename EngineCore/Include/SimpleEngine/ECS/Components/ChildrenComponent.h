#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se
{
/**
 * 현재 Entity의 자식 Entity를 지정합니다.
 */
struct SE_CORE_API ChildrenComponent
{
    Array<Entity> children;
};
} // namespace se

SE_DECLARE_REFLECTION(se::ChildrenComponent, SE_CORE_API)
