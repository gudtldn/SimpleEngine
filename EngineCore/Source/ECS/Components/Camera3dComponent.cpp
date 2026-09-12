#include "SimpleEngine/ECS/Components/Camera3dComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for Camera3dComponent
SE_BEGIN_REFLECT_V1(Camera3dComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(fov, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(near_plane, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(far_plane, meta::Reflect)
SE_END_REFLECT_V1(Camera3dComponent)
}
