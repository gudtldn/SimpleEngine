#include "gtest/gtest.h"

#include "SimpleEditor/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"

using namespace se;
using namespace se::editor;

class MeshAssetTest : public ::testing::Test {};

TEST_F(MeshAssetTest, StaticMeshFactory_IndexedMesh)
{
    StaticMeshFactory factory;
    auto node = std::make_unique<StaticMeshPipelineNode>(Guid::NewGuid());

    // 1. Indexed Triangle
    node->vertices.Push({ .position = { 0, 0, 0 } });
    node->vertices.Push({ .position = { 1, 0, 0 } });
    node->vertices.Push({ .position = { 0, 1, 0 } });
    node->indices.Push(0);
    node->indices.Push(1);
    node->indices.Push(2);

    node->sections.Push({
        .index_offset = 0,
        .index_count = 3,
        .vertex_offset = 0,
        .vertex_count = 3,
        .material_index = 0
    });

    PipelineImportContext ctx;
    auto asset = factory.CreateAsset(node.get(), ctx);
    auto mesh = std::dynamic_pointer_cast<StaticMesh>(asset);

    ASSERT_NE(mesh, nullptr);
    ASSERT_EQ(mesh->lods.Len(), 1);
    ASSERT_EQ(mesh->lods[0].sections.Len(), 1);
    
    const auto& section = mesh->lods[0].sections[0];
    EXPECT_EQ(section.index_count, 3);
    EXPECT_EQ(section.vertex_count, 3);
    
    // Bounds check
    EXPECT_FLOAT_EQ(section.bounds.min.x, 0.0f);
    EXPECT_FLOAT_EQ(section.bounds.max.x, 1.0f);
}

TEST_F(MeshAssetTest, StaticMeshFactory_NonIndexedMesh)
{
    StaticMeshFactory factory;
    auto node = std::make_unique<StaticMeshPipelineNode>(Guid::NewGuid());

    // 2. Non-Indexed Triangles (2 Sections)
    // Section 1: vertices [0, 1, 2]
    node->vertices.Push({ .position = { 0, 0, 0 } });
    node->vertices.Push({ .position = { 1, 0, 0 } });
    node->vertices.Push({ .position = { 0, 1, 0 } });
    
    // Section 2: vertices [3, 4, 5]
    node->vertices.Push({ .position = { 10, 10, 10 } });
    node->vertices.Push({ .position = { 11, 10, 10 } });
    node->vertices.Push({ .position = { 10, 11, 10 } });

    // indices is empty for non-indexed

    node->sections.Push({
        .index_offset = 0,
        .index_count = 0,
        .vertex_offset = 0,
        .vertex_count = 3,
        .material_index = 0
    });

    node->sections.Push({
        .index_offset = 0,
        .index_count = 0,
        .vertex_offset = 3,
        .vertex_count = 3,
        .material_index = 1
    });

    PipelineImportContext ctx;
    auto asset = factory.CreateAsset(node.get(), ctx);
    auto mesh = std::dynamic_pointer_cast<StaticMesh>(asset);

    ASSERT_NE(mesh, nullptr);
    ASSERT_EQ(mesh->lods[0].sections.Len(), 2);

    const auto& s1 = mesh->lods[0].sections[0];
    const auto& s2 = mesh->lods[0].sections[1];

    EXPECT_EQ(s1.index_count, 0);
    EXPECT_EQ(s1.vertex_count, 3);
    EXPECT_EQ(s1.vertex_offset, 0);
    EXPECT_FLOAT_EQ(s1.bounds.min.x, 0.0f);
    EXPECT_FLOAT_EQ(s1.bounds.max.x, 1.0f);

    EXPECT_EQ(s2.index_count, 0);
    EXPECT_EQ(s2.vertex_count, 3);
    EXPECT_EQ(s2.vertex_offset, 3);
    EXPECT_FLOAT_EQ(s2.bounds.min.x, 10.0f);
    EXPECT_FLOAT_EQ(s2.bounds.max.x, 11.0f);
    
    // Global bounds check
    EXPECT_FLOAT_EQ(mesh->bounds.min.x, 0.0f);
    EXPECT_FLOAT_EQ(mesh->bounds.max.x, 11.0f);
}
