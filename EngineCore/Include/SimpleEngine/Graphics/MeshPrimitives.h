#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Math/MathSerialize.h"


namespace se::graphics
{
/**
 * 렌더링에 필요한 기본 정점 데이터
 */
struct alignas(16) Vertex
{
    Vector3f position;
    Vector3f normal;
    Vector2f tex_coord;
    Vector4f tangent;
};

inline void Serialize(Archive& ar, Vertex& v)
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

/**
 * 메쉬의 일부분(서브셋)을 정의하는 데이터
 */
struct MeshSection
{
    uint32 index_start;
    uint32 index_count;
    // uint32 vertex_start; // 나중에 하나의 큰 버퍼로 합칠 때 필요
    uint32 material_index;

    AABBf bounds;
};

inline void Serialize(Archive& ar, MeshSection& s)
{
    ar("index_start") << s.index_start;
    ar("index_count") << s.index_count;
    ar("material_index") << s.material_index;
    ar("bounds") << s.bounds;
}
}  // namespace se::graphics
