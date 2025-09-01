export module SE.Components:Camera3dComponent;

import SE.Math;


/**
 * 3D 카메라의 렌즈 특성(시야각, 클리핑 평면)을 정의하는 컴포넌트
 */
export struct Camera3dComponent
{
    Degree<float> fov = 90.0_degf;
    float near_plane = 0.1f;
    float far_plane = 10'000.0f;
};
