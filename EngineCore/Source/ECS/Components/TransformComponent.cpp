#include "ECS/Components/TransformComponent.h"
#include "Reflection/Reflect.h"

using namespace se;


// Reflection for TransformComponent
SE_BEGIN_REFLECT(TransformComponent, meta::Component)
    SE_REFLECT_PROPERTY(rotation, meta::Edit)
    SE_REFLECT_PROPERTY(position, meta::Edit)
    SE_REFLECT_PROPERTY(scale, meta::Edit)
SE_END_REFLECT(TransformComponent)
