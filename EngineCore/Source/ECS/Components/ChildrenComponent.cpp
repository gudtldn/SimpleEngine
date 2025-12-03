#include "ECS/Components/ChildrenComponent.h"
#include "Reflection/Reflect.h"

// Reflection for ChildrenComponent
SE_BEGIN_REFLECT(ChildrenComponent, meta::Component)
    SE_REFLECT_PROPERTY(children, meta::Edit)
SE_END_REFLECT(ChildrenComponent)
