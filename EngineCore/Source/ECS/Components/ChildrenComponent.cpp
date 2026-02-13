#include "ECS/Components/ChildrenComponent.h"
#include "Core/Reflection/Reflect.h"


namespace se
{
// Reflection for ChildrenComponent
SE_BEGIN_REFLECT(ChildrenComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(children, meta::Property)
SE_END_REFLECT(ChildrenComponent)
}
