#include "gtest/gtest.h"

#include <ranges>

#include "SimpleEditor/Asset/Pipeline/AssetImporter.h"
#include "SimpleEditor/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEditor/Asset/Pipeline/Processors/IPipelineProcessor.h"
#include "SimpleEditor/Asset/ImportSettings/ImportSettingsBase.h"

#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Math/Math.h"

using namespace se;
using namespace se::editor;

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

/**
 * 테스트용 Translator
 * combine_meshes=true  -> 1개 노드 (병합, 1 draw call)
 * combine_meshes=false -> 2개 노드 (분리, 2 draw call)
 */
class MockMeshTranslator : public IPipelineTranslator
{
public:
    [[nodiscard]] virtual ArrayView<const StringView> GetSupportedExtensions() const override
    {
        static constexpr FixedArray<StringView, 1> SUPPORTED_EXTENSIONS = { ".mock" };
        return SUPPORTED_EXTENSIONS;
    }

    virtual void Translate(
        const Path& file_path,
        const ImportProfile& import_profile,
        ImportContext& io_ctx,
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
            // combine_meshes=true: 1개의 병합된 StaticMesh (1 draw call)
            const Guid guid = io_ctx.AllocateSubAssetGuid("MockMesh");
            auto& node = out_container.CreateNode<StaticMeshPipelineNode>(guid);
            node.SetDisplayName("MockMesh");

            StaticVertex v;
            v.position = Vector3f(1.0f, 1.0f, 1.0f);
            v.normal = Vector3f(0.0f, 0.0f, 1.0f);
            v.tex_coord = Vector2f(0.0f, 0.0f);
            v.tangent = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
            node.vertices.Push(v);
            node.indices.Push(0);
        }
        else
        {
            // combine_meshes=false: 2개의 독립된 StaticMesh (per-primitive)
            {
                const Guid guid1 = io_ctx.AllocateSubAssetGuid("Mesh1");
                auto& node1 = out_container.CreateNode<StaticMeshPipelineNode>(guid1);
                node1.SetDisplayName("Mesh1");

                StaticVertex v;
                v.position = Vector3f(1.0f, 1.0f, 1.0f);
                v.normal = Vector3f(0.0f, 0.0f, 1.0f);
                v.tex_coord = Vector2f(0.0f, 0.0f);
                v.tangent = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
                node1.vertices.Push(v);
                node1.indices.Push(0);
            }

            {
                const Guid guid2 = io_ctx.AllocateSubAssetGuid("Mesh2");
                auto& node2 = out_container.CreateNode<StaticMeshPipelineNode>(guid2);
                node2.SetDisplayName("Mesh2");

                StaticVertex v;
                v.position = Vector3f(2.0f, 2.0f, 2.0f);
                v.normal = Vector3f(0.0f, 0.0f, 1.0f);
                v.tex_coord = Vector2f(0.0f, 0.0f);
                v.tangent = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
                node2.vertices.Push(v);
                node2.indices.Push(0);
            }
        }
    }
};

// 테스트용 Processor: MeshScaleProcessor
class MeshScaleProcessor : public IPipelineProcessor
{
public:
    explicit MeshScaleProcessor(f32 in_scale)
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
    f32 scale = 1.0f;
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
    AssetRegistry test_registry;
    HashMap<String, Guid> reserved_guids;
    Array<std::pair<String, Guid>> allocated_guids;

    virtual void SetUp() override
    {
        reserved_guids.Clear();
        allocated_guids.Clear();
    }
};

TEST_F(AssetPipelineTest, ImportPipeline_ScaleProcessorTest)
{
    // combine_meshes=true (기본): 1개의 병합된 StaticMesh가 스케일 처리됨
    AssetImporter importer;
    importer.RegisterTranslator<MockMeshTranslator>();
    importer.RegisterFactory<StaticMeshFactory>();

    PipelineProcessorStack pipeline_stack;
    constexpr f32 SCALE_FACTOR = 100.0f;
    pipeline_stack.AddProcessor<MeshScaleProcessor>(SCALE_FACTOR);

    ImportProfile config;
    ImportContext ctx{ reserved_guids, test_registry, allocated_guids };

    auto assets_exp = importer.Import("Test.mock", ctx, config, pipeline_stack);
    EXPECT_TRUE(assets_exp.HasValue());

    auto assets = std::move(assets_exp).Value();
    ASSERT_FALSE(assets.IsEmpty());
    ASSERT_EQ(assets.GetCount(), 1);

    ASSERT_TRUE(assets.GetMainAsset());
    auto mesh = std::dynamic_pointer_cast<StaticMesh>(assets.GetMainAsset()->asset);
    ASSERT_NE(mesh, nullptr);
    ASSERT_FALSE(mesh->vertices.IsEmpty());

    const auto& pos = mesh->vertices[0].position;
    EXPECT_FLOAT_EQ(pos.x, 1.0f * SCALE_FACTOR);
    EXPECT_FLOAT_EQ(pos.y, 1.0f * SCALE_FACTOR);
    EXPECT_FLOAT_EQ(pos.z, 1.0f * SCALE_FACTOR);
}

TEST_F(AssetPipelineTest, ImportPipeline_CombineMeshesConfigTest)
{
    AssetImporter importer;
    importer.RegisterTranslator<MockMeshTranslator>();
    importer.RegisterFactory<StaticMeshFactory>();

    // combine_meshes=true: 모든 primitive를 1개의 StaticMesh로 병합 (1 draw call)
    {
        ImportProfile config;
        MockImportSettings settings;
        settings.combine_meshes = true;
        config.Set(settings);

        ImportContext ctx{ reserved_guids, test_registry, allocated_guids };
        auto assets = importer.Import("Test.mock", ctx, config);
        EXPECT_TRUE(assets.HasValue());
        ASSERT_EQ(assets->GetCount(), 1);
    }

    // combine_meshes=false: 각 primitive를 독립된 StaticMesh로 분리
    {
        allocated_guids.Clear();

        ImportProfile config;
        MockImportSettings settings;
        settings.combine_meshes = false;
        config.Set(settings);

        ImportContext ctx{ reserved_guids, test_registry, allocated_guids };
        auto assets = importer.Import("Test.mock", ctx, config);
        EXPECT_TRUE(assets.HasValue());
        ASSERT_EQ(assets->GetCount(), 2);

        // 각 에셋이 독립된 정점 데이터를 보유
        auto mesh1 = assets->GetAsset<StaticMesh>("Mesh1");
        auto mesh2 = assets->GetAsset<StaticMesh>("Mesh2");
        ASSERT_NE(mesh1, nullptr);
        ASSERT_NE(mesh2, nullptr);
        EXPECT_FLOAT_EQ(mesh1->vertices[0].position.x, 1.0f);
        EXPECT_FLOAT_EQ(mesh2->vertices[0].position.x, 2.0f);
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
    ImportContext ctx{ reserved_guids, test_registry, allocated_guids };
    auto assets = importer.Import("Test.mock", ctx, config, pipeline_stack);
    EXPECT_TRUE(assets.HasValue());

    // combine_meshes=true (기본): 1개의 병합된 에셋
    ASSERT_EQ(assets->GetCount(), 1);
    ASSERT_TRUE(assets->GetMainAsset());
    auto mesh = std::dynamic_pointer_cast<StaticMesh>(assets->GetMainAsset()->asset);
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
    ImportContext ctx{ reserved_guids, test_registry, allocated_guids };
    // "Empty"가 포함된 파일명은 MockMeshTranslator에서 무시됨
    auto assets = importer.Import("Empty.mock", ctx, config);
    EXPECT_TRUE(assets.HasError());

    // 에러를 반환
    ASSERT_EQ(assets.Error().GetType(), ImportError::FactoryFailed);
}
