#include "SimpleEditor/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"

#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Cast.h"

#include "tracy/Tracy.hpp"


namespace se::editor
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

    // 전체 메쉬 AABB 계산
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

    MeshLOD lod0;
    lod0.screen_size = 1.0f; // 기본 LOD

    // Material Index 매핑용 임시 배열 (중복 방지)
    Array<uint32> unique_materials;

    for (const PipelineMeshSection& pipeline_section : mesh_node->sections)
    {
        MeshSection section;
        section.index_offset = pipeline_section.index_offset;
        section.index_count = pipeline_section.index_count;
        section.vertex_offset = pipeline_section.vertex_offset;

        // Material Index를 0부터 시작하는 연속된 배열 인덱스로 매핑
        if (const auto found_idx = unique_materials.Find(pipeline_section.material_index))
        {
            section.material_slot = static_cast<uint16>(*found_idx);
        }
        else
        {
            section.material_slot = static_cast<uint16>(unique_materials.Len());
            unique_materials.Push(pipeline_section.material_index);
        }

        // 각 섹션별 바운딩 박스 계산
        AABBf section_bounds;
        for (uint32 i = 0; i < section.index_count; ++i)
        {
            const uint32 index = static_mesh->indices[section.index_offset + i];
            const uint32 vertex_index = index + section.vertex_offset;
            section_bounds.Expand(static_mesh->vertices[vertex_index].position);
        }
        section.bounds = section_bounds;

        lod0.sections.Push(section);
    }

    static_mesh->lods.Push(std::move(lod0));

    // 고유 머티리얼 개수만큼 기본 머티리얼 슬롯 할당 (추후 MaterialFactory 연동 시 에셋 ID로 교체)
    // 현재는 아무 것도 연결되지 않은 빈 슬롯을 두어, MeshMaterialComponent가 오버라이드 하거나
    // CollectDrawData에서 Builtin DefaultLitInstance로 자동 폴백되게 만듭니다.
    static_mesh->default_materials.Reserve(unique_materials.Len());
    for (uint32 i = 0; i < unique_materials.Len(); ++i)
    {
        static_mesh->default_materials.Push(AssetId::Invalid);
    }

    (void)context;

    return static_mesh;
}
} // namespace se::editor
