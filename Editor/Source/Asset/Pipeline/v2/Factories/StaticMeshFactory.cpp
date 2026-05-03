#include "SimpleEditor/Asset/Pipeline/v2/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"

#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Cast.h"

#include "tracy/Tracy.hpp"


namespace se::editor::v2
{
TypeId StaticMeshFactory::GetAssetType() const
{
    return TypeId::Get<StaticMesh>();
}

bool StaticMeshFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return IsA<se::editor::StaticMeshPipelineNode>(node);
}

std::shared_ptr<AssetBase> StaticMeshFactory::CreateAsset(
    PipelineBaseNode* node,
    const PipelineImportContext& context
)
{
    ZoneScopedN("StaticMeshFactory::CreateAsset");

    auto* mesh_node = CastChecked<se::editor::StaticMeshPipelineNode>(node);

    auto static_mesh = std::make_shared<StaticMesh>();
    static_mesh->vertices = std::move(mesh_node->vertices);
    static_mesh->indices = std::move(mesh_node->indices);

    static_mesh->bounds = [&]
    {
        ZoneScopedN("Calculate Mesh AABB");

        AABBf mesh_bounds;
        for (const StaticVertex& vertex : static_mesh->vertices)
        {
            mesh_bounds.Expand(vertex.position);
        }
        return mesh_bounds;
    }();

    // TODO (Phase 2): mesh_node->material_index로 sibling MaterialInstance AssetId 연결
    (void)context;

    return static_mesh;
}
} // namespace se::editor::v2
