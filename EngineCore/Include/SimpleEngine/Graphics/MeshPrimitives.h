#pragma once
#include "SimpleEngine/Core/Math/Math.h"


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

/**
 * 스킨/애니메이션 데이터 (SkeletalMesh 전용)
 */
struct SkinVertex
{
    uint32 bone_indices[4]; // 최대 4개의 뼈가 영향
    float bone_weights[4]; // 각 뼈의 가중치 (총합 1.0, uint8[4] (0~255 정규화)로도 가능)
};

/**
 * 메쉬의 일부분(서브셋)을 정의하는 데이터
 */
struct MeshSection
{
    uint32 index_start;
    uint32 index_count;
    // uint32 vertex_start; // 나중에 하나의 큰 버퍼로 합칠 때 필요

    uint32 material_index;
    // TODO: AABB bounding_box; 추가하기
};
}  // namespace se::graphics
