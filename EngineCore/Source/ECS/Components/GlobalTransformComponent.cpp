#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"

#include "SimpleEngine/Core/Math/MathSerialize.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT(GlobalTransformComponent, meta::Reflect, meta::Transient, meta::Component)
    SE_REFLECT_PROPERTY(value, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT(GlobalTransformComponent)
}
