#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Math/MathSerialize.h"


namespace se::graphics
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

    friend void Serialize(Archive& ar, StaticVertex& v)
    {
        ar("position") << v.position;
        ar("normal") << v.normal;
        ar("tex_coord") << v.tex_coord;
        ar("tangent") << v.tangent;
    }
};

/**
 * 스킨/애니메이션 데이터 (SkeletalMesh 전용)
 */
struct SkinVertex
{
    FixedArray<uint32, 4> bone_indices; // 최대 4개의 뼈가 영향
    FixedArray<float, 4> bone_weights;  // 각 뼈의 가중치 (총합 1.0)

    friend void Serialize(Archive& ar, SkinVertex& v)
    {
        ar("bone_indices") << v.bone_indices;
        ar("bone_weights") << v.bone_weights;
    }
};


/**
 * 머티리얼 경계로 나뉜 StaticMesh의 논리적 서브메시 단위
 */
struct MeshSection
{
    uint32 index_offset = 0;   // 전체 indices 배열 내 시작 인덱스
    uint32 index_count = 0;    // 이 섹션의 인덱스 수
    uint32 vertex_offset = 0;  // 베이스 버텍스 오프셋 (SDL base vertex)
    uint32 material_index = 0; // StaticMesh::materials 배열 참조 인덱스

    friend void Serialize(Archive& ar, MeshSection& s)
    {
        ar("index_offset") << s.index_offset;
        ar("index_count") << s.index_count;
        ar("vertex_offset") << s.vertex_offset;
        ar("material_index") << s.material_index;
    }
};

} // namespace se::graphics
