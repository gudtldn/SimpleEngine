#include "Asset/Pipeline/Factories/StaticMeshFactory.h"

#include "Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "Asset/Types/MeshTypes.h"


namespace se::asset
{
TypeId StaticMeshFactory::GetAssetType() const
{
    return TypeId::Get<StaticMesh>();
}

bool StaticMeshFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return node->GetTypeId() == TypeId::Get<StaticMeshPipelineNode>();
}

std::shared_ptr<IAsset> StaticMeshFactory::CreateAsset(
    PipelineBaseNode* node,
    const PipelineImportContext& context
)
{
    StaticMeshPipelineNode* mesh_node = static_cast<StaticMeshPipelineNode*>(node);

    auto static_mesh = std::make_shared<StaticMesh>();
    static_mesh->vertices = std::move(mesh_node->vertices);
    static_mesh->indices = std::move(mesh_node->indices);
    static_mesh->sections = std::move(mesh_node->sections);

    // TODO: AABB 계산
    // CalculateBounds(static_mesh);

    // TODO: 머티리얼 연결
    // for (const auto& section : static_mesh->sections)
    // {
    //      // context.GetCreatedAsset(...)를 통해 머티리얼 에셋을 찾아서 연결
    // }
    (void)context;

    return static_mesh;
}
}  // namespace se::asset
