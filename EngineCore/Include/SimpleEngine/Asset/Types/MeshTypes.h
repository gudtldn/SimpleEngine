#pragma once
#include "SimpleEngine/Asset/Types/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "SimpleEngine/Meta/Annotations.h"


namespace se::asset
{
/**
 * @todo docs
 */
struct SE_CORE_API StaticMesh : Asset<StaticMesh>
{
    // 추후 Depth Prepass를 위한 Position정보와 나머지 Vertex정보를 분리해서
    // 하이브리드 SoA 방식으로 구조를 변경해 볼 수도 있음.
    // (GPU Buffer를 positions +(pad) attributes +(pad) indices로 할당해서, Buffer를 2개로 나눠 Slot0, 1에 할당)
    //
    // struct graphics::VertexAttributes
    // {
    //     Vector3f normal;
    //     Vector2f tex_coord;
    //     Vector4f tangent;
    // }
    //
    // SE_PROPERTY(=::se::meta::ReadOnly)
    // Array<Vector3f> positions;
    //
    // SE_PROPERTY(=::se::meta::ReadOnly)
    // Array<graphics::VertexAttributes> attributes;

    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<graphics::Vertex> vertices;

    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<uint32> indices;

    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<graphics::MeshSection> sections;

    // SE_PROPERTY(=::se::meta::ReadOnly)
    // Array<Material> materials;

    // TODO: AABB bounds; 추가
};

/**
 * @todo docs
 */
struct SE_CORE_API SkeletalMesh : Asset<SkeletalMesh>
{
    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<graphics::Vertex> vertices;

    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<graphics::SkinVertex> skin_vertices;

    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<uint32> indices;

    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<graphics::MeshSection> sections;

    // SE_PROPERTY(=::se::meta::ReadOnly)
    // Array<Material> materials;

    // 뼈대 정보 (계층 구조, InverseBindPose 등)는 별도 구조체로 관리
    // Array<BoneInfo> ref_skeleton;
};
}  // namespace se::asset
