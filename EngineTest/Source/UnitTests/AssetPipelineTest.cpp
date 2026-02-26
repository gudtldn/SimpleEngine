#include "gtest/gtest.h"

#include <filesystem>
#include <ranges>

#include "SimpleEditor/Asset/Pipeline/AssetImporter.h"
#include "SimpleEditor/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEditor/Asset/Pipeline/Processors/IPipelineProcessor.h"
#include "SimpleEditor/Asset/ImportSettings/ImportSettingsBase.h"

#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Math/Math.h"

using namespace se;
using namespace se::asset;
using namespace se::editor;
using namespace se::graphics;

// --- Mock Classes ---

// 테스트용 Import Settings
struct SE_ANNOTATION(=meta::Reflect) MockImportSettings : public ImportSettingsBase
{
    SE_CLASS(MockImportSettings, ImportSettingsBase)

public:
    SE_ANNOTATION(=meta::Property)
    bool combine_meshes = true;
};

SE_BEGIN_REFLECT(MockImportSettings, meta::Reflect)
    SE_REFLECT_PROPERTY(combine_meshes, meta::Property)
SE_END_REFLECT(MockImportSettings)

// 테스트용 간단한 Translator
class MockMeshTranslator : public IPipelineTranslator
{
public:
    [[nodiscard]] virtual ArrayView<const StringView> GetSupportedExtensions() const override
    {
        static constexpr FixedArray<StringView, 1> exts = { ".mock" };
        return exts;
    }

    virtual void Translate(
        const Path& file_path,
        const ImportProfile& import_profile,
        PipelineNodeContainer& out_container
    ) override
    {
        // 예외 처리 테스트: 특정 파일명이면 아무것도 생성하지 않음
        if (file_path.FileStem() == "Empty")
        {
            return;
        }

        auto settings = import_profile.GetOrDefault<MockImportSettings>();

        if (settings.combine_meshes)
        {
            // 간단한 큐브 형태 혹은 단일 점을 가진 StaticMeshNode 생성
            auto& node = out_container.CreateNode<StaticMeshPipelineNode>();
            node.SetDisplayName("MockMesh");

            // 정점 하나 추가 (1, 1, 1)
            Vertex v;
            v.position = Vector3f(1.0f, 1.0f, 1.0f);
            v.normal = Vector3f(0.0f, 0.0f, 1.0f);
            v.tex_coord = Vector2f(0.0f, 0.0f);
            v.tangent = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);

            node.vertices.Push(v);
            node.indices.Push(0);

            MeshSection section;
            section.index_start = 0;
            section.index_count = 1;
            section.material_index = 0;
            node.sections.Push(section);
        }
        else
        {
            // combine_meshes = false 인 경우: 두 개의 노드로 분리
            {
                auto& node1 = out_container.CreateNode<StaticMeshPipelineNode>();
                node1.SetDisplayName("Mesh1");

                Vertex v;
                v.position = Vector3f(1.0f, 1.0f, 1.0f);
                node1.vertices.Push(v);
                node1.indices.Push(0);

                MeshSection section;
                section.index_start = 0;
                section.index_count = 1;
                section.material_index = 0;
                node1.sections.Push(section);
            }

            {
                auto& node2 = out_container.CreateNode<StaticMeshPipelineNode>();
                node2.SetDisplayName("Mesh2");

                Vertex v;
                v.position = Vector3f(2.0f, 2.0f, 2.0f);
                node2.vertices.Push(v);
                node2.indices.Push(0);

                MeshSection section;
                section.index_start = 0;
                section.index_count = 1;
                section.material_index = 0;
                node2.sections.Push(section);
            }
        }
    }
};

// 테스트용 Processor: MeshScaleProcessor
class MeshScaleProcessor : public IPipelineProcessor
{
public:
    explicit MeshScaleProcessor(float in_scale)
        : scale(in_scale)
    {
    }

    virtual void Process(PipelineNodeContainer& in_out_container) override
    {
        for (const auto& node_ptr : in_out_container.GetAllNodes() | std::views::values)
        {
            // StaticMeshPipelineNode인지 확인
            if (node_ptr->GetTypeId() == TypeId::Get<StaticMeshPipelineNode>())
            {
                // 다운캐스팅
                auto* mesh_node = static_cast<StaticMeshPipelineNode*>(node_ptr.get());

                for (auto& vertex : mesh_node->vertices)
                {
                    vertex.position *= scale;
                }
            }
        }
    }

private:
    float scale = 1.0f;
};

// 테스트용 Processor: MeshOffsetProcessor
class MeshOffsetProcessor : public IPipelineProcessor
{
public:
    explicit MeshOffsetProcessor(const Vector3f& in_offset)
        : offset(in_offset)
    {
    }

    virtual void Process(PipelineNodeContainer& in_out_container) override
    {
        for (const auto& node_ptr : in_out_container.GetAllNodes() | std::views::values)
        {
            if (node_ptr->GetTypeId() == TypeId::Get<StaticMeshPipelineNode>())
            {
                auto* mesh_node = static_cast<StaticMeshPipelineNode*>(node_ptr.get());
                for (auto& vertex : mesh_node->vertices)
                {
                    vertex.position += offset;
                }
            }
        }
    }

private:
    Vector3f offset;
};

// --- Tests ---

class AssetPipelineTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
    }
};

TEST_F(AssetPipelineTest, ImportPipeline_ScaleProcessorTest)
{
    AssetImporter importer;
    importer.RegisterTranslator<MockMeshTranslator>();
    importer.RegisterFactory<StaticMeshFactory>();

    PipelineProcessorStack pipeline_stack;
    constexpr float scale_factor = 100.0f;
    pipeline_stack.AddProcessor<MeshScaleProcessor>(scale_factor);

    ImportProfile config;

    auto assets_exp = importer.Import("Test.mock", config, pipeline_stack);
    EXPECT_TRUE(assets_exp.HasValue());

    auto assets = std::move(assets_exp).Value();
    ASSERT_FALSE(assets.IsEmpty());
    ASSERT_EQ(assets.GetCount(), 1);

    const auto& asset = assets.GetAsset(0);
    ASSERT_NE(asset, nullptr);

    auto mesh = std::dynamic_pointer_cast<StaticMesh>(asset);
    ASSERT_NE(mesh, nullptr);

    ASSERT_FALSE(mesh->vertices.IsEmpty());
    ASSERT_GE(mesh->vertices.Len(), 1);

    const auto& pos = mesh->vertices[0].position;
    EXPECT_FLOAT_EQ(pos.x, 1.0f * scale_factor);
    EXPECT_FLOAT_EQ(pos.y, 1.0f * scale_factor);
    EXPECT_FLOAT_EQ(pos.z, 1.0f * scale_factor);
}

TEST_F(AssetPipelineTest, ImportPipeline_ConfigTest)
{
    // Config 테스트: ImportProfile에 combine_meshes = false를 넣었을 때 동작 확인
    AssetImporter importer;
    importer.RegisterTranslator<MockMeshTranslator>();
    importer.RegisterFactory<StaticMeshFactory>();

    // 1. combine_meshes = false
    {
        ImportProfile config;
        MockImportSettings settings;
        settings.combine_meshes = false;
        config.Set(settings);

        auto assets = importer.Import("Test.mock", config);
        EXPECT_TRUE(assets.HasValue());

        // 두 개의 노드가 생성되어 두 개의 에셋이 반환되어야 함
        ASSERT_EQ(assets->GetCount(), 2);
    }

    // 2. combine_meshes = true (기본값)
    {
        ImportProfile config;
        MockImportSettings settings;
        settings.combine_meshes = true;
        config.Set(settings);

        auto assets = importer.Import("Test.mock", config);
        EXPECT_TRUE(assets.HasValue());

        // 하나의 통합된 노드가 생성되어 하나의 에셋이 반환되어야 함
        ASSERT_EQ(assets->GetCount(), 1);
    }
}

TEST_F(AssetPipelineTest, ImportPipeline_MultiProcessorTest)
{
    // 다중 Processor 테스트: ScaleProcessor -> OffsetProcessor
    AssetImporter importer;
    importer.RegisterTranslator<MockMeshTranslator>();
    importer.RegisterFactory<StaticMeshFactory>();

    PipelineProcessorStack pipeline_stack;
    // 먼저 2배 확대
    pipeline_stack.AddProcessor<MeshScaleProcessor>(2.0f);
    // 그 다음 (1, 0, 0) 만큼 이동
    pipeline_stack.AddProcessor<MeshOffsetProcessor>(Vector3f(1.0f, 0.0f, 0.0f));

    ImportProfile config;
    auto assets = importer.Import("Test.mock", config, pipeline_stack);
    EXPECT_TRUE(assets.HasValue());

    ASSERT_EQ(assets->GetCount(), 1);
    auto mesh = std::dynamic_pointer_cast<StaticMesh>(assets->GetAsset(0));
    ASSERT_NE(mesh, nullptr);

    // 원본 (1, 1, 1) -> Scale(2배) -> (2, 2, 2) -> Offset(+1, 0, 0) -> (3, 2, 2)
    const auto& pos = mesh->vertices[0].position;
    EXPECT_FLOAT_EQ(pos.x, 3.0f);
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(pos.z, 2.0f);
}

TEST_F(AssetPipelineTest, ImportPipeline_EmptyResultTest)
{
    // 예외 처리 테스트: Translator가 빈 컨테이너를 반환했을 때
    AssetImporter importer;
    importer.RegisterTranslator<MockMeshTranslator>();
    importer.RegisterFactory<StaticMeshFactory>();

    ImportProfile config;
    // "Empty"가 포함된 파일명은 MockMeshTranslator에서 무시됨
    auto assets = importer.Import("Empty.mock", config);
    EXPECT_TRUE(assets.HasError());

    // 에러를 반환
    ASSERT_EQ(assets.Error().GetCode(), ImportError::ECode::FactoryFailed);
}
