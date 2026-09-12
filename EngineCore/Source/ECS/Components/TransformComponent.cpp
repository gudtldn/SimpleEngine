#include "SimpleEngine/ECS/Components/TransformComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for TransformComponent
SE_BEGIN_REFLECT_V1(TransformComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(rotation, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(position, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(scale, meta::Reflect)
SE_END_REFLECT_V1(TransformComponent)
}
