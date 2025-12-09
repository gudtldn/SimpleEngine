#pragma once
#include "SimpleEngine/Asset/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Reflection/Annotations.h"


namespace se::asset
{
/**
 * 렌더링에 필요한 기본 정점 데이터
 */
struct Vertex
{
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    Vector2f tex_coord;
};

/**
 * 스킨/애니메이션 데이터 (SkeletalMesh 전용)
 */
struct SkinVertex
{
    int32 bone_indices[1]; // 최대 4개의 뼈가 영향
    float bone_weights[1]; // 각 뼈의 가중치
};

/**
 * 메쉬의 일부분(서브셋)을 정의하는 데이터
 */
struct MeshSection
{
    uint32 material_index;
    uint32 index_start;
    uint32 index_count;
    // uint32 vertex_start; // 나중에 하나의 큰 버퍼로 합칠 때 필요
};

/**
 * @todo docs
 */
struct SE_CORE_API StaticMesh : IAsset
{
    SE_PROPERTY(=meta::ReadOnly)
    Array<Vertex> vertices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<uint32> indices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<MeshSection> sections;
};

/**
 * @todo docs
 */
struct SE_CORE_API SkeletalMesh : IAsset
{
    SE_PROPERTY(=meta::ReadOnly)
    Array<Vertex> vertices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<SkinVertex> skin_vertices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<uint32> indices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<MeshSection> sections;

    // 뼈대 정보 (계층 구조, InverseBindPose 등)는 별도 구조체로 관리
    // Array<BoneInfo> ref_skeleton;
};
}  // namespace se::asset
