#include "ECS/Components/ChildrenComponent.h"
#include "Core/Reflection/Reflect.h"

using namespace se;


// Reflection for ChildrenComponent
SE_BEGIN_REFLECT(ChildrenComponent, ::se::meta::Component)
    SE_REFLECT_PROPERTY(children, ::se::meta::Edit)
SE_END_REFLECT(ChildrenComponent)
