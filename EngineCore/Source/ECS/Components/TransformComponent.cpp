#include "SimpleEngine/ECS/Components/TransformComponent.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for TransformComponent
SE_BEGIN_REFLECT(TransformComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(rotation, meta::Reflect)
    SE_REFLECT_PROPERTY(position, meta::Reflect)
    SE_REFLECT_PROPERTY(scale, meta::Reflect)
SE_END_REFLECT(TransformComponent)
}
