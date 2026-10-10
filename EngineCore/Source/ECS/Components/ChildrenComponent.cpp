#include "SimpleEngine/ECS/Components/ChildrenComponent.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::ChildrenComponent, se::ecs::Component)
    SE_FIELD(children)
SE_REFLECT_END()
