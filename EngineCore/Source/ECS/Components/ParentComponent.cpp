#include "SimpleEngine/ECS/Components/ParentComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for ParentComponent
SE_BEGIN_REFLECT_V1(ParentComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(parent, meta::Reflect)
SE_END_REFLECT_V1(ParentComponent)
}
