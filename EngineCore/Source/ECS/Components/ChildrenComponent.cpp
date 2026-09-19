#include "SimpleEngine/ECS/Components/ChildrenComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for ChildrenComponent
SE_BEGIN_REFLECT_V1(ChildrenComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(children, meta::Reflect)
SE_END_REFLECT_V1(ChildrenComponent)
}
