#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"


namespace se::editor
{
/** @todo docs */
struct PipelineMeshSection
{
    u32 index_offset = 0;
    u32 index_count = 0;
    i32 vertex_offset = 0;
    u32 vertex_count = 0;
    u32 material_index = 0;
};

/**
 * @todo docs
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) StaticMeshPipelineNode : public PipelineBaseNode
{
    SE_CLASS(StaticMeshPipelineNode, PipelineBaseNode)

public:
    // Mesh Data
    Array<StaticVertex> vertices;
    Array<u32> indices;
    Array<PipelineMeshSection> sections;

    // 노드의 로컬 트랜스폼 (Z-up 엔진 convention)
    // apply_transform=false일 때 Assimp 노드의 변환된 로컬 트랜스폼을 보존
    Matrix4x4f local_transform = Matrix4x4f::Identity();

    // aiMaterial 인덱스 -> PipelineMaterialInstanceNode UID 매핑 (AssimpTranslator에서 채움)
    // 인덱스 범위: [0, scene->mNumMaterials)
    Array<Guid> material_node_uids;
};
} // namespace se::editor
