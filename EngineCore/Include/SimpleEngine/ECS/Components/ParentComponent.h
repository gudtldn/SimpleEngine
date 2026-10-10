#pragma once

#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se
{
/**
 * 현재 Entity의 부모 Entity를 지정합니다.
 */
struct SE_CORE_API ParentComponent
{
public:
    Entity parent;
};
} // namespace se

SE_DECLARE_REFLECTION(se::ParentComponent, SE_CORE_API)
