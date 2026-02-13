#include "SimpleEngine/Asset/Pipeline/Factories/StaticMeshFactory.h"

#include "SimpleEngine/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Cast.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
TypeId StaticMeshFactory::GetAssetType() const
{
    return TypeId::Get<StaticMesh>();
}

bool StaticMeshFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return IsA<StaticMeshPipelineNode>(node);
}

std::shared_ptr<AssetBase> StaticMeshFactory::CreateAsset(
    PipelineBaseNode* node,
    const PipelineImportContext& context
)
{
    ZoneScopedN("StaticMeshFactory::CreateAsset");

    StaticMeshPipelineNode* mesh_node = CastChecked<StaticMeshPipelineNode>(node);

    auto static_mesh = std::make_shared<StaticMesh>();
    static_mesh->vertices = std::move(mesh_node->vertices);
    static_mesh->indices = std::move(mesh_node->indices);
    static_mesh->sections = std::move(mesh_node->sections);

    // 전체 메쉬 AABB 계산
    static_mesh->bounds = [&]
    {
        ZoneScopedN("Calculate Mesh AABB");

        AABBf mesh_bounds;
        for (const auto& vertex : static_mesh->vertices)
        {
            mesh_bounds.Expand(vertex.position);
        }
        return mesh_bounds;
    }();

    // 섹션별 AABB 계산
    for (auto& section : static_mesh->sections)
    {
        section.bounds = [&]
        {
            ZoneScopedN("Calculate Section AABB");

            AABBf section_bounds;
            const uint32 end = section.index_start + section.index_count;
            for (uint32 i = section.index_start; i < end; ++i)
            {
                section_bounds.Expand(static_mesh->vertices[static_mesh->indices[i]].position);
            }
            return section_bounds;
        }();
    }

    // TODO: 머티리얼 연결
    // for (const auto& section : static_mesh->sections)
    // {
    //      // context.GetCreatedAsset(...)를 통해 머티리얼 에셋을 찾아서 연결
    // }
    (void)context;

    return static_mesh;
}
}  // namespace se::asset
