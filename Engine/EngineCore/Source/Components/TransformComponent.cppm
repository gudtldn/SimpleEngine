export module SimpleEngine.Components:TransformComponent;

import SimpleEngine.Math;


/**
 * 3D 공간에서 Entity의 위치, 회전, 크기를 정의하는 컴포넌트
 */
export struct TransformComponent
{
    Vector3 position = Vector3::Zero();
    Quaternion rotation = Quaternion::Identity();
    Vector3 scale = Vector3::One();
};
