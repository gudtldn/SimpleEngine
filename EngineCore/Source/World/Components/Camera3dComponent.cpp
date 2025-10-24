#include "World/Components/Camera3dComponent.h"
#include "Reflection/Reflect.h"

// Reflection for Camera3dComponent
SE_BEGIN_REFLECT(Camera3dComponent)
    SE_REFLECT_PROPERTY(fov)
    SE_REFLECT_PROPERTY(near_plane)
    SE_REFLECT_PROPERTY(far_plane)
SE_END_REFLECT(Camera3dComponent)
