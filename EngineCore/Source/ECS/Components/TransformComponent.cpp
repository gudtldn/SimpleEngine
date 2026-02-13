#include "ECS/Components/TransformComponent.h"
#include "Core/Reflection/Reflect.h"


namespace se
{
// Reflection for TransformComponent
SE_BEGIN_REFLECT(TransformComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(rotation, meta::Property)
    SE_REFLECT_PROPERTY(position, meta::Property)
    SE_REFLECT_PROPERTY(scale, meta::Property)
SE_END_REFLECT(TransformComponent)
}
