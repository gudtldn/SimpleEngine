#include "SimpleEngine/ECS/Components/Camera3dComponent.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for Camera3dComponent
SE_BEGIN_REFLECT(Camera3dComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(fov, meta::Reflect)
    SE_REFLECT_PROPERTY(near_plane, meta::Reflect)
    SE_REFLECT_PROPERTY(far_plane, meta::Reflect)
SE_END_REFLECT(Camera3dComponent)
}
