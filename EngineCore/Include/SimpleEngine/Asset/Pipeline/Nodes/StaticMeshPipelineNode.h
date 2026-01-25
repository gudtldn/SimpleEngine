#pragma once
#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineNode.h"
#include "SimpleEngine/Gfx/MeshPrimitives.h"


namespace se::asset
{
class SE_CORE_API StaticMeshPipelineNode : public PipelineNode<StaticMeshPipelineNode>
{
public:
    // Mesh Data
    Array<gfx::Vertex> vertices;
    Array<uint32> indices;
    Array<gfx::MeshSection> sections;

    // 이 메쉬가 참조하는 머티리얼 노드들의 UID 목록 (나중에 Material Factory와 연결용)
    // Array<Guid> material_dependencies;
};
}  // namespace se::asset
