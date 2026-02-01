#include "ECS/Components/ParentComponent.h"
#include "Core/Reflection/Reflect.h"

using namespace se;


// Reflection for ParentComponent
SE_BEGIN_REFLECT(ParentComponent, ::se::meta::Component)
    SE_REFLECT_PROPERTY(parent, ::se::meta::Edit)
SE_END_REFLECT(ParentComponent)
