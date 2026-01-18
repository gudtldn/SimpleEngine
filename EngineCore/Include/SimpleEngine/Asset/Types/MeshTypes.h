#pragma once
#include "SimpleEngine/Asset/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Gfx/MeshPrimitives.h"
#include "SimpleEngine/Reflection/Annotations.h"


namespace se::asset
{
/**
 * @todo docs
 */
struct SE_CORE_API StaticMesh : IAsset
{
    SE_PROPERTY(=meta::ReadOnly)
    Array<gfx::Vertex> vertices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<uint32> indices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<gfx::MeshSection> sections;

    // SE_PROPERTY(=meta::ReadOnly)
    // Array<Material> materials;

    // TODO: AABB bounds; 추가
};

/**
 * @todo docs
 */
struct SE_CORE_API SkeletalMesh : IAsset
{
    SE_PROPERTY(=meta::ReadOnly)
    Array<gfx::Vertex> vertices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<gfx::SkinVertex> skin_vertices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<uint32> indices;

    SE_PROPERTY(=meta::ReadOnly)
    Array<gfx::MeshSection> sections;

    // SE_PROPERTY(=meta::ReadOnly)
    // Array<Material> materials;

    // 뼈대 정보 (계층 구조, InverseBindPose 등)는 별도 구조체로 관리
    // Array<BoneInfo> ref_skeleton;
};
}  // namespace se::asset
