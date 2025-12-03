#include "ECS/Components/ParentComponent.h"
#include "Reflection/Reflect.h"

// Reflection for ParentComponent
SE_BEGIN_REFLECT(ParentComponent, meta::Component)
    SE_REFLECT_PROPERTY(parent, meta::Edit)
SE_END_REFLECT(ParentComponent)
