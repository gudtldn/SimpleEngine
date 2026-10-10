#include "SimpleEngine/ECS/Components/Camera3dComponent.h"

#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::Camera3dComponent, se::ecs::Component)
    SE_FIELD(fov)
    SE_FIELD(near_plane)
    SE_FIELD(far_plane)
SE_REFLECT_END()
