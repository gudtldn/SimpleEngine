#include "SimpleEditor/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"

#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Cast.h"

#include "tracy/Tracy.hpp"

#include <ranges>


namespace se::editor
{
TypeId StaticMeshFactory::GetAssetType() const
{
    return TypeId::Get<asset::StaticMesh>();
}

bool StaticMeshFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return IsA<StaticMeshPipelineNode>(node);
}

std::shared_ptr<asset::AssetBase> StaticMeshFactory::CreateAsset(
    PipelineBaseNode* node,
    const PipelineImportContext& context
)
{
    ZoneScopedN("StaticMeshFactory::CreateAsset");

    StaticMeshPipelineNode* mesh_node = CastChecked<StaticMeshPipelineNode>(node);

    auto static_mesh = std::make_shared<asset::StaticMesh>();
    static_mesh->vertices = std::move(mesh_node->vertices);
    static_mesh->indices = std::move(mesh_node->indices);

    // 전체 메쉬 AABB 계산
    static_mesh->bounds = [&]
    {
        ZoneScopedN("Calculate Mesh AABB");

        AABBf mesh_bounds;
        for (const graphics::StaticVertex& vertex : static_mesh->vertices)
        {
            mesh_bounds.Expand(vertex.position);
        }
        return mesh_bounds;
    }();

    // MeshSection 및 머티리얼 연결
    // for (uint32 i = 0; i < mesh_node->sections.Len(); ++i)
    for (const auto [idx, section] : mesh_node->sections | std::views::enumerate)
    {
        graphics::MeshSection dst = {
            .index_offset = section.index_offset,
            .index_count = section.index_count,
            .vertex_offset = section.vertex_offset,
            .material_index = static_cast<uint32>(idx),
        };

        static_mesh->materials.Push(
            section.material_node_uid.IsValid()
                ? context.GetCreatedAssetId(section.material_node_uid).ValueOr(asset::AssetId::Invalid)
                : asset::AssetId::Invalid
        );

        static_mesh->sections.Push(dst);
    }

    return static_mesh;
}
} // namespace se::editor
