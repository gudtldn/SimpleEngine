#include "SimpleEditor/Camera/EditorCameraState.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se::editor
{
using namespace se::math;

SE_BEGIN_REFLECT_V1(EditorCameraState, meta::Reflect, meta::Transient)
    SE_REFLECT_PROPERTY_V1(position, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(rotation, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(velocity, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(ortho_width, meta::Reflect, meta::Range(0.1f, 10000.0f))
    SE_REFLECT_PROPERTY_V1(fov_y, meta::Reflect, meta::Range(0.0f, 180.0f))
    SE_REFLECT_PROPERTY_V1(near_plane, meta::Reflect, meta::Range(0.001f, 100.0f))
    SE_REFLECT_PROPERTY_V1(far_plane, meta::Reflect, meta::Range(1.0f, 100'000.0f))
    SE_REFLECT_PROPERTY_V1(move_speed, meta::Reflect, meta::Range(0.01f, 1000.0f))
    SE_REFLECT_PROPERTY_V1(look_sensitivity, meta::Reflect, meta::Range(0.001f, 10.0f))
SE_END_REFLECT_V1(EditorCameraState)
} // namespace se::editor
