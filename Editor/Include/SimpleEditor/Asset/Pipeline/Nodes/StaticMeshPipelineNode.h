#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Graphics/MeshPrimitives.h"


namespace se::editor
{
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) StaticMeshPipelineNode : public PipelineBaseNode
{
    SE_CLASS(StaticMeshPipelineNode, PipelineBaseNode)

public:
    // Mesh Data
    Array<graphics::Vertex> vertices;
    Array<uint32> indices;

    // Assimp의 mMaterialIndex 보존 (Material Factory 구현 시 활용)
    uint32 material_index = 0;
};
} // namespace se::editor
