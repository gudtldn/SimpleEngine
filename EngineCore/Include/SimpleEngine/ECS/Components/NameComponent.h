#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * Entity에 이름을 지정하는 컴포넌트
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) NameComponent
{
    SE_ANNOTATION(=meta::Reflect)
    String name;
};
} // namespace se

SE_DECLARE_REFLECTION(se::NameComponent)
