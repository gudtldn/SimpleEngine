#include "gtest/gtest.h"

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"

#include <cstring>
#include <tuple>

#if SE_HAS_HLSL_COMPILER

using namespace se;
using namespace se::editor;

// Slang 컴파일 결과의 블롭, SDL 슬롯 계산, 상수 버퍼 레이아웃, 의존 파일, 실패 종류 검증
namespace
{
constexpr u32 SPIRV_MAGIC = 0x07230203;

/** 테스트 전체가 함께 쓰는 컴파일러. 전역 세션 생성이 무거워 한 번만 만듭니다. */
[[nodiscard]] const ShaderCompiler& GetCompiler()
{
    static const ShaderCompiler compiler;
    return compiler;
}

[[nodiscard]] Path FixtureDirectory()
{
    return Path(SE_TEST_SOURCE_DIR) / "EngineTest/Shaders";
}

[[nodiscard]] ShaderCookResult<CompiledShaderProgram> CompileFixture(const char* file_name)
{
    return GetCompiler().Compile({ .source_path = FixtureDirectory() / file_name, .include_dirs = { FixtureDirectory() } });
}

[[nodiscard]] Optional<const ShaderResourceSlot&> FindSlot(const Array<ShaderResourceSlot>& slots, const char* name)
{
    const StringName key = name;
    return slots.FindBy([&key](const ShaderResourceSlot& slot) { return slot.name == key; });
}

[[nodiscard]] Optional<const ShaderUniformMember&> FindMember(const ShaderUniformBuffer& buffer, const char* name)
{
    const StringName key = name;
    return buffer.members.FindBy([&key](const ShaderUniformMember& member) { return member.name == key; });
}

[[nodiscard]] Optional<const ShaderBlob&> FindBlob(const CompiledShaderStage& stage, EShaderFormat format)
{
    return stage.blobs.FindBy([format](const ShaderBlob& blob) { return blob.format == format; });
}

[[nodiscard]] u32 ReadU32(const Array<u8>& code)
{
    u32 value = 0;
    std::memcpy(&value, code.Data(), sizeof(value));
    return value;
}
} // namespace

TEST(ShaderCompilerTest, ProducesSpirvAndDxilBlobsPerStage)
{
    const auto result = CompileFixture("Fixture_TwoStages.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    ASSERT_EQ(result->stages.Len(), 2u);
    for (const CompiledShaderStage& stage : result->stages)
    {
        const char* expected_entry = stage.stage == EShaderStage::Vertex ? "VSMain" : "PSMain";
        ASSERT_EQ(stage.blobs.Len(), 2u);

        const auto spirv = FindBlob(stage, EShaderFormat::SPIRV);
        ASSERT_TRUE(spirv.HasValue());
        EXPECT_EQ(spirv->stage, stage.stage);
        EXPECT_EQ(spirv->entry_point, expected_entry);
        ASSERT_GE(spirv->code.Len(), 4u);
        EXPECT_EQ(ReadU32(spirv->code), SPIRV_MAGIC);

        const auto dxil = FindBlob(stage, EShaderFormat::DXIL);
        ASSERT_TRUE(dxil.HasValue());
        EXPECT_EQ(dxil->stage, stage.stage);
        EXPECT_EQ(dxil->entry_point, expected_entry);
        ASSERT_GE(dxil->code.Len(), 4u);
        EXPECT_EQ(std::memcmp(dxil->code.Data(), "DXBC", 4), 0);
    }
}

TEST(ShaderCompilerTest, StageInterfaceExcludesOtherStageResources)
{
    const auto result = CompileFixture("Fixture_TwoStages.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const auto vertex = result->FindStage(EShaderStage::Vertex);
    ASSERT_TRUE(vertex.HasValue());
    const ShaderStageInterface& vs = vertex->stage_interface;
    EXPECT_EQ(vs.counts, (ShaderResourceCounts{ .uniform_buffers = 2 }));
    EXPECT_TRUE(vs.sampled_textures.IsEmpty());
    EXPECT_TRUE(vs.samplers.IsEmpty());
    ASSERT_EQ(vs.uniform_buffers.Len(), 2u);
    EXPECT_EQ(vs.uniform_buffers[0].name, StringName("PassUBO"));
    EXPECT_EQ(vs.uniform_buffers[0].slot, 0u);
    EXPECT_EQ(vs.uniform_buffers[1].name, StringName("ObjectUBO"));
    EXPECT_EQ(vs.uniform_buffers[1].slot, 1u);

    const auto fragment = result->FindStage(EShaderStage::Fragment);
    ASSERT_TRUE(fragment.HasValue());
    const ShaderStageInterface& ps = fragment->stage_interface;
    EXPECT_EQ(ps.counts, (ShaderResourceCounts{ .samplers = 1, .uniform_buffers = 1 }));
    ASSERT_EQ(ps.uniform_buffers.Len(), 1u);
    EXPECT_EQ(ps.uniform_buffers[0].name, StringName("MaterialUBO"));
    EXPECT_TRUE(FindSlot(ps.sampled_textures, "base_color_texture").HasValue());
    EXPECT_TRUE(FindSlot(ps.samplers, "base_color_sampler").HasValue());

    // 필터 전 선언은 두 스테이지 것을 모두 담습니다.
    EXPECT_EQ(result->declared_bindings.Len(), 5u);
}

TEST(ShaderCompilerTest, ReflectsVertexInputsAndVaryings)
{
    const auto result = CompileFixture("Fixture_TwoStages.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const auto vertex = result->FindStage(EShaderStage::Vertex);
    ASSERT_TRUE(vertex.HasValue());
    const Array<ShaderVertexInput>& inputs = vertex->stage_interface.vertex_inputs;
    ASSERT_EQ(inputs.Len(), 2u);
    EXPECT_EQ(inputs[0], (ShaderVertexInput{ .location = 0, .name = "position", .type = EShaderValueType::Float3 }));
    EXPECT_EQ(inputs[1], (ShaderVertexInput{ .location = 1, .name = "tex_coord", .type = EShaderValueType::Float2 }));

    // SV_Position은 보간 값이 아니므로 빠집니다.
    ASSERT_EQ(vertex->outputs.Len(), 2u);
    EXPECT_EQ(vertex->outputs[0].semantic, "TEXCOORD0");
    EXPECT_EQ(vertex->outputs[0].type, EShaderValueType::Float2);
    EXPECT_EQ(vertex->outputs[1].semantic, "TEXCOORD1");
    EXPECT_EQ(vertex->outputs[1].type, EShaderValueType::UInt);

    const auto fragment = result->FindStage(EShaderStage::Fragment);
    ASSERT_TRUE(fragment.HasValue());
    EXPECT_TRUE(fragment->stage_interface.vertex_inputs.IsEmpty());
    ASSERT_EQ(fragment->inputs.Len(), 2u);
    EXPECT_EQ(fragment->inputs[0].semantic, "TEXCOORD0");
    EXPECT_EQ(fragment->inputs[1].semantic, "TEXCOORD1");
    EXPECT_TRUE(fragment->outputs.IsEmpty());
}

TEST(ShaderCompilerTest, SamplerCountCoversSlotHole)
{
    const auto result = CompileFixture("Fixture_SlotHole.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const auto fragment = result->FindStage(EShaderStage::Fragment);
    ASSERT_TRUE(fragment.HasValue());
    const ShaderStageInterface& ps = fragment->stage_interface;
    EXPECT_EQ(ps.counts.samplers, 5u);

    const auto first = FindSlot(ps.sampled_textures, "first_texture");
    const auto fifth = FindSlot(ps.sampled_textures, "fifth_texture");
    ASSERT_TRUE(first.HasValue());
    ASSERT_TRUE(fifth.HasValue());
    EXPECT_EQ(first->slot, 0u);
    EXPECT_EQ(fifth->slot, 4u);
}

TEST(ShaderCompilerTest, UniformLayoutMatchesAcrossFormats)
{
    const auto result = CompileFixture("Fixture_Layout.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const auto fragment = result->FindStage(EShaderStage::Fragment);
    ASSERT_TRUE(fragment.HasValue());
    ASSERT_EQ(fragment->stage_interface.uniform_buffers.Len(), 1u);
    const ShaderUniformBuffer& spirv = fragment->stage_interface.uniform_buffers[0];

    constexpr std::tuple<const char*, u32, EShaderValueType> EXPECTED_MEMBERS[] = {
        { "a", 0, EShaderValueType::Float },  { "b", 4, EShaderValueType::Float2 },    { "c", 12, EShaderValueType::Float },
        { "d", 16, EShaderValueType::Float3 }, { "e", 32, EShaderValueType::Float },   { "f", 52, EShaderValueType::Float },
        { "g", 64, EShaderValueType::Float4x4 }, { "h", 128, EShaderValueType::UInt },
    };
    for (const auto& [name, offset, type] : EXPECTED_MEMBERS)
    {
        const auto member = FindMember(spirv, name);
        ASSERT_TRUE(member.HasValue()) << name;
        EXPECT_EQ(member->offset, offset) << name;
        EXPECT_EQ(member->type, type) << name;
    }

    const auto e = FindMember(spirv, "e");
    ASSERT_TRUE(e.HasValue());
    EXPECT_EQ(e->array_count, 2u);

    ASSERT_EQ(fragment->dxil_uniform_buffers.Len(), 1u);
    EXPECT_EQ(fragment->dxil_uniform_buffers[0], spirv);
}

TEST(ShaderCompilerTest, StorageBuffersFollowTextureRange)
{
    const auto result = CompileFixture("Fixture_Storage.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const auto vertex = result->FindStage(EShaderStage::Vertex);
    ASSERT_TRUE(vertex.HasValue());
    EXPECT_EQ(vertex->stage_interface.counts, (ShaderResourceCounts{ .storage_buffers = 1 }));
    const auto offsets = FindSlot(vertex->stage_interface.storage_buffers, "vertex_offsets");
    ASSERT_TRUE(offsets.HasValue());
    EXPECT_EQ(offsets->slot, 0u);

    const auto fragment = result->FindStage(EShaderStage::Fragment);
    ASSERT_TRUE(fragment.HasValue());
    const ShaderStageInterface& ps = fragment->stage_interface;
    EXPECT_EQ(ps.counts, (ShaderResourceCounts{ .samplers = 2, .storage_buffers = 2 }));

    const auto lights = FindSlot(ps.storage_buffers, "lights");
    const auto light_indices = FindSlot(ps.storage_buffers, "light_indices");
    ASSERT_TRUE(lights.HasValue());
    ASSERT_TRUE(light_indices.HasValue());
    EXPECT_EQ(lights->slot, 0u);
    EXPECT_EQ(light_indices->slot, 1u);
}

TEST(ShaderCompilerTest, ReflectsComputeSetsAndThreadCount)
{
    const auto result = CompileFixture("Fixture_Compute.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    ASSERT_EQ(result->stages.Len(), 1u);
    const auto compute = result->FindStage(EShaderStage::Compute);
    ASSERT_TRUE(compute.HasValue());
    const ShaderStageInterface& cs = compute->stage_interface;

    EXPECT_EQ(cs.thread_count, (ShaderThreadCount{ .x = 8, .y = 8, .z = 1 }));
    EXPECT_EQ(cs.counts, (ShaderResourceCounts{
        .samplers = 1,
        .storage_buffers = 1,
        .readwrite_storage_textures = 1,
        .readwrite_storage_buffers = 1,
        .uniform_buffers = 1,
    }));

    const auto input_particles = FindSlot(cs.storage_buffers, "input_particles");
    const auto output_texture = FindSlot(cs.readwrite_storage_textures, "output_texture");
    const auto output_particles = FindSlot(cs.readwrite_storage_buffers, "output_particles");
    ASSERT_TRUE(input_particles.HasValue());
    ASSERT_TRUE(output_texture.HasValue());
    ASSERT_TRUE(output_particles.HasValue());
    EXPECT_EQ(input_particles->slot, 0u);
    EXPECT_EQ(output_texture->slot, 0u);
    EXPECT_EQ(output_particles->slot, 0u);

    ASSERT_EQ(compute->blobs.Len(), 2u);
    for (const ShaderBlob& blob : compute->blobs)
    {
        EXPECT_EQ(blob.stage, EShaderStage::Compute);
        EXPECT_EQ(blob.entry_point, "CSMain");
    }
}

TEST(ShaderCompilerTest, ListsIncludedFilesAsDependencies)
{
    const auto result = CompileFixture("Fixture_Include.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const auto has_file = [&result](const char* file_name)
    {
        return result->dependencies
            .FindBy([file_name](const Path& path)
            {
                const auto name = path.FileName();
                return name.HasValue() && *name == file_name;
            })
            .HasValue();
    };
    EXPECT_TRUE(has_file("Fixture_Include.hlsl"));
    EXPECT_TRUE(has_file("Fixture_Common.hlsli"));
}

TEST(ShaderCompilerTest, FailsWithoutEntryPoint)
{
    const auto result = CompileFixture("Fixture_NoEntry.hlsl");

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().GetType(), ShaderCookError::NoEntryPoint);
}

TEST(ShaderCompilerTest, FailsOnSyntaxErrorWithDiagnostics)
{
    const auto result = CompileFixture("Fixture_SyntaxError.hlsl");

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().GetType(), ShaderCookError::CompileFailed);
    EXPECT_NE(std::strlen(result.Error().What()), 0u);
}

#endif
