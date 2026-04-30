#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"


namespace se::editor
{
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) StaticMeshPipelineNode : public PipelineBaseNode
{
    SE_CLASS(StaticMeshPipelineNode, PipelineBaseNode)

public:
    /** 하나의 서브 메시 범위와 연결된 머티리얼 노드 UID를 담는 서술자 */
    struct SubMeshSection
    {
        uint32 index_offset = 0;
        uint32 index_count = 0;
        uint32 vertex_offset = 0;

        // AssimpMaterialExtractor가 생성한 PipelineMaterialNode의 UID
        Guid material_node_uid;
    };

public:
    virtual void GetFactoryDependencies(Array<Guid>& out_dependencies) const override
    {
        PipelineBaseNode::GetFactoryDependencies(out_dependencies);
        for (const SubMeshSection& section : sections)
        {
            if (section.material_node_uid.IsValid())
            {
                out_dependencies.Push(section.material_node_uid);
            }
        }
    }

public:
    // Mesh Data (모든 서브 메시가 병합된 버퍼)
    Array<graphics::StaticVertex> vertices;
    Array<uint32> indices;

    // 서브 메시 섹션 목록
    Array<SubMeshSection> sections;

    // 노드의 로컬 트랜스폼 (Z-up 엔진 convention)
    // apply_transform=false일 때 Assimp 노드의 변환된 로컬 트랜스폼을 보존
    Matrix4x4f local_transform = Matrix4x4f::Identity();
};
} // namespace se::editor
