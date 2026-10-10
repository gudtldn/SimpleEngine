#include "SimpleEngine/ECS/Components/TransformComponent.h"

#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::TransformComponent, se::ecs::Component)
    SE_FIELD(rotation)
    SE_FIELD(position)
    SE_FIELD(scale)
    SE_FIELD(dirty)
SE_REFLECT_END()
