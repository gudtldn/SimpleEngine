#include "SimpleEditor/Camera/EditorCameraState.h"
#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Transient.h"


SE_REFLECT_BEGIN(se::editor::EditorCameraState, se::serde::Transient)
    SE_FIELD(position)
    SE_FIELD(rotation)
    SE_FIELD(velocity)
    SE_FIELD(ortho_width)
    SE_FIELD(fov_y)
    SE_FIELD(near_plane)
    SE_FIELD(far_plane)
    SE_FIELD(move_speed)
    SE_FIELD(look_sensitivity)
SE_REFLECT_END()
