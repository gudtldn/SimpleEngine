#include "SimpleEngine/Graphics/View/RenderView.h"

#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_BEGIN(se::RenderView)
    SE_FIELD(camera_pos)
    SE_FIELD(view_matrix)
    SE_FIELD(projection_matrix)
    SE_FIELD(width)
    SE_FIELD(height)
    SE_FIELD(near_plane)
    SE_FIELD(far_plane)
    SE_FIELD(fov_y)
    SE_FIELD(rendering_mode)
    SE_FIELD(show_flags)
SE_REFLECT_END()
