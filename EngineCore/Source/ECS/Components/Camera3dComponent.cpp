#include "ECS/Components/Camera3dComponent.h"
#include "Core/Reflection/Reflect.h"


namespace se
{
// Reflection for Camera3dComponent
SE_BEGIN_REFLECT(Camera3dComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(fov, meta::Property)
    SE_REFLECT_PROPERTY(near_plane, meta::Property)
    SE_REFLECT_PROPERTY(far_plane, meta::Property)
SE_END_REFLECT(Camera3dComponent)
}
