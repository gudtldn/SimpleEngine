#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"

#include "SimpleEngine/Core/Math/MathSerialize.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
SE_BEGIN_REFLECT_V1(GlobalTransformComponent, meta::Reflect, meta::Transient, meta::Component)
    SE_REFLECT_PROPERTY_V1(value, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT_V1(GlobalTransformComponent)
}
