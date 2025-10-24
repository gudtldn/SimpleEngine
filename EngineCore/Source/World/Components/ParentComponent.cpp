#include "World/Components/ParentComponent.h"
#include "Reflection/Reflect.h"

// Reflection for ParentComponent
SE_BEGIN_REFLECT(ParentComponent)
    SE_REFLECT_PROPERTY(parent)
SE_END_REFLECT(ParentComponent)