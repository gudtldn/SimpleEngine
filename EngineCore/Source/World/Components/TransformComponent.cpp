#include "World/Components/TransformComponent.h"
#include "Reflection/Reflect.h"

// Reflection for TransformComponent
SE_BEGIN_REFLECT(TransformComponent)
    SE_REFLECT_PROPERTY(rotation)
    SE_REFLECT_PROPERTY(position)
    SE_REFLECT_PROPERTY(scale)
SE_END_REFLECT(TransformComponent)