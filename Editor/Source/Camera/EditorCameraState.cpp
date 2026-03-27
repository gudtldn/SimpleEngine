#include "SimpleEditor/Camera/EditorCameraState.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
using namespace se::math;

SE_BEGIN_REFLECT(EditorCameraState, meta::EditorOnly)
    SE_REFLECT_PROPERTY(position, meta::Property)
    SE_REFLECT_PROPERTY(rotation, meta::Property)
    SE_REFLECT_PROPERTY(velocity, meta::Property)
    SE_REFLECT_PROPERTY(fov_y, meta::Property, meta::Range(0.0f, 180.0f))
    SE_REFLECT_PROPERTY(near_plane, meta::Property, meta::Range(0.001f, 100.0f))
    SE_REFLECT_PROPERTY(far_plane, meta::Property, meta::Range(1.0f, 100'000.0f))
    SE_REFLECT_PROPERTY(move_speed, meta::Property, meta::Range(0.01f, 1000.0f))
    SE_REFLECT_PROPERTY(look_sensitivity, meta::Property, meta::Range(0.001f, 10.0f))
SE_END_REFLECT(EditorCameraState)

graphics::RenderView EditorCameraState::ComputeRenderView(uint32 width, uint32 height) const
{
    const Vector3 forward = rotation.GetForwardVector();
    const Vector3 target = position + forward;

    const double aspect = (height > 0)
        ? static_cast<double>(width) / static_cast<double>(height)
        : 1.0;
    const Radian<double> fov_rad{ fov_y };

    return {
        .view_matrix = TransformUtility::MakeViewMatrix(position, target, Vector3::Up()),
        .projection_matrix = TransformUtility::MakePerspectiveMatrix(fov_rad, aspect, near_plane, far_plane),
        .width = width,
        .height = height,
        .near_plane = static_cast<float>(near_plane),
        .far_plane = static_cast<float>(far_plane),
    };
}
} // namespace se::editor
