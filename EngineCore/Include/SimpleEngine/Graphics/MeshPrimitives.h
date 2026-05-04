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
 * @note 나중에 GPU Culling이나 Indirect Rendering을 할 때 이 단위로 판별합니다.
 */
struct MeshSection
{
    /** 전체 indices 배열 내에서 이 Section이 시작하는 인덱스 위치 */
    uint32 index_offset = 0;

    /** 이 Section이 그릴 인덱스의 총 개수 */
    uint32 index_count = 0;

    /**
     * 베이스 버텍스 오프셋 (SDL base vertex / first vertex)
     * @note API 규격 상 int32 필수 (Vulkan: vertexOffset, SDL3: base_vertex)
     *       인덱스 버퍼에서 읽은 값에 더해져 최종 버텍스 주소를 결정하며, 음수 시프팅이 가능해야 하기 때문.
     *       비-인덱스 드로우(index_count == 0)일 때는 시작 버텍스 오프셋(first_vertex)으로 사용됩니다.
     */
    int32 vertex_offset = 0;

    /** 이 Section이 그릴 버텍스의 총 개수 (비-인덱스 드로우 시에만 사용) */
    uint32 vertex_count = 0;

    /** StaticMesh::default_materials 배열의 참조 인덱스 */
    uint32 material_slot = 0;

    /** 이 Section의 바운딩 박스 */
    AABBf bounds;

    friend void Serialize(Archive& ar, MeshSection& s)
    {
        ar("index_offset") << s.index_offset;
        ar("index_count") << s.index_count;
        ar("vertex_offset") << s.vertex_offset;
        ar("vertex_count") << s.vertex_count;
        ar("material_slot") << s.material_slot;
        ar("bounds") << s.bounds;
    }
};

/**
 * 거리에 따라 렌더링 폴리곤 수를 줄이기 위한 LOD 계층
 */
struct MeshLOD
{
    float screen_size = 1.0f;    // 이 LOD가 활성화될 화면 차지 비율 (1.0 = 최대 크기)
    Array<MeshSection> sections; // 이 LOD에 속한 서브메시 섹션들

    friend void Serialize(Archive& ar, MeshLOD& l)
    {
        ar("screen_size") << l.screen_size;
        ar("sections") << l.sections;
    }
};
} // namespace se
