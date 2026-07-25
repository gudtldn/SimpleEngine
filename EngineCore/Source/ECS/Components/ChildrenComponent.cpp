#include "SimpleEngine/ECS/Components/ChildrenComponent.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for ChildrenComponent
SE_BEGIN_REFLECT(ChildrenComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(children, meta::Reflect)
SE_END_REFLECT(ChildrenComponent)
}
