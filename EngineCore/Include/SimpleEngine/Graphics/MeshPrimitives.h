#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Math/MathSerialize.h"


namespace se
{
/**
 * 렌더링에 필요한 기본 정점 데이터
 */
struct alignas(16) StaticVertex
{
    Vector3f position;
    Vector3f normal;
    Vector2f tex_coord;
    Vector4f tangent;
};

inline void Serialize(Archive& ar, StaticVertex& v)
{
    ar("position") << v.position;
    ar("normal") << v.normal;
    ar("tex_coord") << v.tex_coord;
    ar("tangent") << v.tangent;
}

/**
 * 스킨/애니메이션 데이터 (SkeletalMesh 전용)
 */
struct SkinVertex
{
    FixedArray<uint32, 4> bone_indices; // 최대 4개의 뼈가 영향
    FixedArray<float, 4> bone_weights;  // 각 뼈의 가중치 (총합 1.0)
};

inline void Serialize(Archive& ar, SkinVertex& v)
{
    ar("bone_indices") << v.bone_indices;
    ar("bone_weights") << v.bone_weights;
}
} // namespace se
