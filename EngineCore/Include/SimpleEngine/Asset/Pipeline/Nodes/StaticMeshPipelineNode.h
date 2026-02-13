#pragma once
#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineBaseNode.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"


namespace se::asset
{
class SE_CORE_API SE_ANNOTATION(=meta::Internal) StaticMeshPipelineNode : public PipelineBaseNode
{
    SE_CLASS(StaticMeshPipelineNode, PipelineBaseNode)

public:
    // Mesh Data
    Array<graphics::Vertex> vertices;
    Array<uint32> indices;
    Array<graphics::MeshSection> sections;

    // 이 메쉬가 참조하는 머티리얼 노드들의 UID 목록 (나중에 Material Factory와 연결용)
    // Array<Guid> material_dependencies;
};
}  // namespace se::asset
