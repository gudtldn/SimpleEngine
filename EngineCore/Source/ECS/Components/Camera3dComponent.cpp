#include "ECS/Components/Camera3dComponent.h"
#include "Reflection/Reflect.h"

using namespace se;


// Reflection for Camera3dComponent
SE_BEGIN_REFLECT(Camera3dComponent, meta::Component)
    SE_REFLECT_PROPERTY(fov, meta::Edit)
    SE_REFLECT_PROPERTY(near_plane, meta::Edit)
    SE_REFLECT_PROPERTY(far_plane, meta::Edit)
SE_END_REFLECT(Camera3dComponent)
