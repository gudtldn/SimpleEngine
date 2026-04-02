#include "SimpleEditor/Camera/EditorCameraState.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
using namespace se::math;

SE_BEGIN_REFLECT(EditorCameraState, meta::EditorOnly)
    SE_REFLECT_PROPERTY(position, meta::Property)
    SE_REFLECT_PROPERTY(rotation, meta::Property)
    SE_REFLECT_PROPERTY(velocity, meta::Property)
    SE_REFLECT_PROPERTY(ortho_width, meta::Property, meta::Range(0.1f, 10000.0f))
    SE_REFLECT_PROPERTY(fov_y, meta::Property, meta::Range(0.0f, 180.0f))
    SE_REFLECT_PROPERTY(near_plane, meta::Property, meta::Range(0.001f, 100.0f))
    SE_REFLECT_PROPERTY(far_plane, meta::Property, meta::Range(1.0f, 100'000.0f))
    SE_REFLECT_PROPERTY(move_speed, meta::Property, meta::Range(0.01f, 1000.0f))
    SE_REFLECT_PROPERTY(look_sensitivity, meta::Property, meta::Range(0.001f, 10.0f))
SE_END_REFLECT(EditorCameraState)
} // namespace se::editor
