#include "SimpleEditor/Camera/EditorCameraState.h"


namespace se::editor
{
using namespace se::math;

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
