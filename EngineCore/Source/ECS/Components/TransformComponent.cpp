#include "ECS/Components/TransformComponent.h"
#include "Core/Reflection/Reflect.h"

using namespace se;


// Reflection for TransformComponent
SE_BEGIN_REFLECT(TransformComponent, ::se::meta::Component)
    SE_REFLECT_PROPERTY(rotation, ::se::meta::Edit)
    SE_REFLECT_PROPERTY(position, ::se::meta::Edit)
    SE_REFLECT_PROPERTY(scale, ::se::meta::Edit)
SE_END_REFLECT(TransformComponent)
