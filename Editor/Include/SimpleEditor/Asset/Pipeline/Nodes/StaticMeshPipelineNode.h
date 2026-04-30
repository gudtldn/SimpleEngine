#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"


namespace se::editor
{
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) StaticMeshPipelineNode : public PipelineBaseNode
{
    SE_CLASS(StaticMeshPipelineNode, PipelineBaseNode)

public:
    // Mesh Data
    Array<StaticVertex> vertices;
    Array<uint32> indices;

    // Assimp의 mMaterialIndex 보존 (Material Factory 구현 시 활용)
    uint32 material_index = 0;

    // 노드의 로컬 트랜스폼 (Z-up 엔진 convention)
    // apply_transform=false일 때 Assimp 노드의 변환된 로컬 트랜스폼을 보존
    Matrix4x4f local_transform = Matrix4x4f::Identity();
};
} // namespace se::editor
