#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/View/ViewSettings.h"


namespace se::graphics
{
/**
 * 단일 뷰포트의 렌더링에 필요한 카메라 정보를 담는 순수 POD 구조체
 *
 * @todo 역행렬(inv_view, inv_vp)과 camera_pos를 멤버로 캐싱하면
 *       DeprojectToRay/ProjectWorldToScreen 호출 시 중복 계산을 제거할 수 있음.
 *       현재는 프레임당 1~2회 호출이라 무시 가능하나, 다수 뷰포트 동시 처리 시 고려.
 */
struct RenderView
{
    Matrix4x4 view_matrix = Matrix4x4::Identity();
    Matrix4x4 projection_matrix = Matrix4x4::Identity();

    uint32 width = 0;
    uint32 height = 0;

    float near_plane = 0.1f;
    float far_plane = 1000.0f;

    ERenderingMode rendering_mode = ERenderingMode::Lit;
    ShowFlags show_flags = EShowFlag::All;

public:
    /**
     * 2D 픽셀 좌표를 기반으로 카메라 원점에서 씬으로 쏘는 3D Ray를 생성합니다. (Screen -> NDC -> Deproject)
     * @param in_cursor_pos 뷰포트 로컬 좌표 (좌상단 원점, px)
     * @return 카메라 위치에서 커서 방향으로 쏘는 정규화된 Ray
     */
    [[nodiscard]] Ray DeprojectToRay(const Vector2f& in_cursor_pos) const
    {
        if (width == 0 || height == 0)
        {
            return Ray{ Vector3::Zero(), Vector3::Forward() };
        }

        // Screen (Pixel) -> NDC 변환
        const double ndc_x = 2.0 * static_cast<double>(in_cursor_pos.x) / static_cast<double>(width) - 1.0;
        const double ndc_y = 1.0 - 2.0 * static_cast<double>(in_cursor_pos.y) / static_cast<double>(height);

        const Matrix4x4 inv_vp = (view_matrix * projection_matrix).Inverse();

        // Near/Far 평면의 두 점을 Deproject하여 Ray 방향을 계산
        // Perspective: near 점 ~ camera 부근, 방향이 커서마다 다름 (방사형)
        // Ortho: near/far 점이 평행 이동, 동일 방향 (평행 ray)
        Vector4 world_near = Vector4{ ndc_x, ndc_y, 0.0, 1.0 } * inv_vp;
        Vector4 world_far = Vector4{ ndc_x, ndc_y, 1.0, 1.0 } * inv_vp;

        world_near = world_near / world_near.w;
        world_far = world_far / world_far.w;

        const Vector3 origin = Vector3{ world_near.x, world_near.y, world_near.z };
        const Vector3 far_point = Vector3{ world_far.x, world_far.y, world_far.z };
        const Vector3 direction = (far_point - origin).GetNormalized();

        return Ray{ origin, direction };
    }

    /**
     * 월드 좌표를 뷰포트 로컬 픽셀 좌표로 투영합니다. (World -> Clip -> NDC -> Screen)
     * @param world_point 월드 공간 좌표
     * @return 뷰포트 로컬 좌표 (좌상단 원점, px). 카메라 뒤에 있으면 결과가 부정확할 수 있음.
     */
    [[nodiscard]] Vector2f ProjectWorldToScreen(const Vector3& world_point) const
    {
        if (width == 0 || height == 0)
        {
            return Vector2f::Zero();
        }

        // Row-vector 연산: clip = point * VP
        const Matrix4x4 vp = view_matrix * projection_matrix;
        const Vector4 clip = Vector4{ world_point, 1.0 } * vp;

        if (math::IsNearlyZero(clip.w))
        {
            return Vector2f::Zero();
        }

        // Perspective divide -> NDC
        const double ndc_x = clip.x / clip.w;
        const double ndc_y = clip.y / clip.w;

        // NDC -> Screen (Pixel)
        return Vector2f{
            static_cast<float>((ndc_x + 1.0) * 0.5 * static_cast<double>(width)),
            static_cast<float>((1.0 - ndc_y) * 0.5 * static_cast<double>(height))
        };
    }
};
} // namespace se::graphics
