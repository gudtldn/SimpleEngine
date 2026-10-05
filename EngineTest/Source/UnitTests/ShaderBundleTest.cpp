#include "gtest/gtest.h"

#include "SimpleEngine/Graphics/Shader/ShaderBundle.h"

using namespace se;

// ShaderBundle 직렬화 왕복, 손상 거부, 블롭과 스테이지 조회 검증
namespace
{
/** 모든 필드에 기본값이 아닌 값을 채운 번들을 만듭니다. */
[[nodiscard]] ShaderBundle MakeFullBundle()
{
    ShaderStageInterface vertex{ .stage = EShaderStage::Vertex };
    vertex.vertex_inputs.Push({ .location = 0, .name = "position", .type = EShaderValueType::Float3 });
    vertex.vertex_inputs.Push({ .location = 2, .name = "tex_coord", .type = EShaderValueType::Float2 });

    ShaderUniformBuffer pass{ .name = "PassUBO", .slot = 0, .size = 64 };
    pass.members.Push({ .name = "vp", .offset = 0, .size = 64, .type = EShaderValueType::Float4x4 });
    vertex.uniform_buffers.Push(std::move(pass));
    vertex.counts = { .uniform_buffers = 1 };

    ShaderStageInterface fragment{ .stage = EShaderStage::Fragment };
    fragment.sampled_textures.Push({ .name = "base_color_texture", .slot = 0 });
    fragment.sampled_textures.Push({ .name = "emissive_texture", .slot = 4 });
    fragment.samplers.Push({ .name = "base_color_sampler", .slot = 0 });
    fragment.storage_buffers.Push({ .name = "lights", .slot = 0 });

    ShaderUniformBuffer material{ .name = "MaterialUBO", .slot = 1, .size = 48 };
    material.members.Push({ .name = "base_color_factor", .offset = 0, .size = 16, .type = EShaderValueType::Float4 });
    material.members.Push({ .name = "weights", .offset = 16, .size = 32, .type = EShaderValueType::Float, .array_count = 2 });
    fragment.uniform_buffers.Push(std::move(material));
    fragment.counts = { .samplers = 5, .storage_textures = 0, .storage_buffers = 1, .uniform_buffers = 2 };

    ShaderBundle bundle;
    bundle.program.stages.Push(std::move(vertex));
    bundle.program.stages.Push(std::move(fragment));
    bundle.blobs.Push({ .stage = EShaderStage::Vertex, .format = EShaderFormat::SPIRV, .entry_point = "VSMain", .code = { 0x03, 0x02, 0x23, 0x07 } });
    bundle.blobs.Push({ .stage = EShaderStage::Vertex, .format = EShaderFormat::DXIL, .entry_point = "VSMain", .code = { 'D', 'X', 'B', 'C' } });
    bundle.blobs.Push({ .stage = EShaderStage::Fragment, .format = EShaderFormat::SPIRV, .entry_point = "PSMain", .code = { 0x03, 0x02, 0x23, 0x07, 0xFF } });
    bundle.blobs.Push({ .stage = EShaderStage::Fragment, .format = EShaderFormat::DXIL, .entry_point = "PSMain", .code = { 'D', 'X', 'B', 'C', 0x00 } });
    bundle.dependencies = { "CoreShader://Default.hlsl", "CoreShader://Bindings.hlsli" };
    return bundle;
}
} // namespace

TEST(ShaderBundleTest, RoundTripPreservesEveryField)
{
    const ShaderBundle original = MakeFullBundle();

    const auto restored = ShaderBundle::Deserialize(original.Serialize());

    ASSERT_TRUE(restored.HasValue()) << restored.Error().CStr();
    EXPECT_EQ(*restored, original);
}

TEST(ShaderBundleTest, RoundTripPreservesEmptyBundle)
{
    const ShaderBundle original;

    const auto restored = ShaderBundle::Deserialize(original.Serialize());

    ASSERT_TRUE(restored.HasValue()) << restored.Error().CStr();
    EXPECT_EQ(*restored, original);
}

TEST(ShaderBundleTest, RejectsTruncatedBytes)
{
    const Array<u8> bytes = MakeFullBundle().Serialize();
    const ArrayView<const u8> truncated(bytes.Data(), bytes.Len() / 2);

    const auto restored = ShaderBundle::Deserialize(truncated);

    EXPECT_FALSE(restored.HasValue());
}

TEST(ShaderBundleTest, RejectsCorruptedPayload)
{
    Array<u8> bytes = MakeFullBundle().Serialize();
    // payload 마지막 바이트를 바꾸면 헤더의 체크섬과 맞지 않습니다.
    bytes[bytes.Len() - 1] ^= 0xFF;

    const auto restored = ShaderBundle::Deserialize(bytes);

    EXPECT_FALSE(restored.HasValue());
}

TEST(ShaderBundleTest, RejectsEmptyBytes)
{
    const auto restored = ShaderBundle::Deserialize({});

    EXPECT_FALSE(restored.HasValue());
}

TEST(ShaderBundleTest, FindBlobMatchesStageAndFormat)
{
    ShaderBundle bundle = MakeFullBundle();
    // 프래그먼트 DXIL을 빼서 "스테이지는 있지만 포맷이 없는" 경우를 만듭니다.
    bundle.blobs.RemoveAt(3);

    const auto vertex_dxil = bundle.FindBlob(EShaderStage::Vertex, EShaderFormat::DXIL);
    ASSERT_TRUE(vertex_dxil.HasValue());
    EXPECT_EQ(vertex_dxil->stage, EShaderStage::Vertex);
    EXPECT_EQ(vertex_dxil->format, EShaderFormat::DXIL);
    EXPECT_EQ(vertex_dxil->entry_point, "VSMain");

    EXPECT_FALSE(bundle.FindBlob(EShaderStage::Fragment, EShaderFormat::DXIL).HasValue());
}

TEST(ShaderBundleTest, FindStageReturnsMatchingInterface)
{
    const ShaderBundle bundle = MakeFullBundle();

    const auto fragment = bundle.program.FindStage(EShaderStage::Fragment);

    ASSERT_TRUE(fragment.HasValue());
    EXPECT_EQ(fragment->counts.samplers, 5u);
    EXPECT_EQ(fragment->sampled_textures.Len(), 2u);
}

TEST(ShaderBundleTest, FindStageReturnsNullOptForMissingStage)
{
    ShaderProgramInterface program;
    program.stages.Push({ .stage = EShaderStage::Vertex });

    EXPECT_FALSE(program.FindStage(EShaderStage::Fragment).HasValue());
}
