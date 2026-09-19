#include "SimpleEngine/ECS/Components/NameComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for NameComponent
SE_BEGIN_REFLECT_V1(NameComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(name, meta::Reflect)
SE_END_REFLECT_V1(NameComponent)
}
