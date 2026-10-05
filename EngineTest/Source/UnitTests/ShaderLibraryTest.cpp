#include "gtest/gtest.h"

#include "SimpleEngine/Shader/ShaderLibrary.h"

using namespace se;

// 번들에서 SDL 생성 정보를 만드는 부분 검증 (GPU 장치 없이)
namespace
{
/** VS는 두 포맷, PS는 SPIR-V만 가진 번들 */
[[nodiscard]] ShaderBundle MakeGraphicsBundle()
{
    ShaderBundle bundle;
    bundle.program.stages.Push({ .stage = EShaderStage::Vertex, .counts = { .uniform_buffers = 2 } });
    bundle.program.stages.Push({ .stage = EShaderStage::Fragment, .counts = { .samplers = 5, .storage_textures = 1, .storage_buffers = 3, .uniform_buffers = 2 } });
    bundle.blobs.Push({ .stage = EShaderStage::Vertex, .format = EShaderFormat::SPIRV, .entry_point = "VSMain", .code = { 0x03, 0x02, 0x23, 0x07 } });
    bundle.blobs.Push({ .stage = EShaderStage::Vertex, .format = EShaderFormat::DXIL, .entry_point = "VSMain", .code = { 'D', 'X', 'B', 'C', 0x00 } });
    bundle.blobs.Push({ .stage = EShaderStage::Fragment, .format = EShaderFormat::SPIRV, .entry_point = "PSMain", .code = { 0x03, 0x02, 0x23, 0x07, 0xFF, 0xFF } });
    return bundle;
}
} // namespace

TEST(ShaderLibraryTest, ChooseBlobPrefersDxil)
{
    const ShaderBundle bundle = MakeGraphicsBundle();

    const auto blob = ShaderLibrary::ChooseBlob(bundle, EShaderStage::Vertex, SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL);
    ASSERT_TRUE(blob.HasValue());
    EXPECT_EQ(blob->format, EShaderFormat::DXIL);
}

TEST(ShaderLibraryTest, ChooseBlobFallsBackToFormatInBundle)
{
    const ShaderBundle bundle = MakeGraphicsBundle();

    // 장치가 DXIL을 받아도 번들에 없으면 SPIR-V를 고릅니다.
    const auto blob = ShaderLibrary::ChooseBlob(bundle, EShaderStage::Fragment, SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL);
    ASSERT_TRUE(blob.HasValue());
    EXPECT_EQ(blob->format, EShaderFormat::SPIRV);
}

TEST(ShaderLibraryTest, ChooseBlobFailsWithoutCommonFormat)
{
    const ShaderBundle bundle = MakeGraphicsBundle();

    EXPECT_FALSE(ShaderLibrary::ChooseBlob(bundle, EShaderStage::Fragment, SDL_GPU_SHADERFORMAT_DXIL).HasValue());
    EXPECT_FALSE(ShaderLibrary::ChooseBlob(bundle, EShaderStage::Vertex, SDL_GPU_SHADERFORMAT_MSL).HasValue());
}

TEST(ShaderLibraryTest, ShaderCreateInfoUsesBundleContract)
{
    const ShaderBundle bundle = MakeGraphicsBundle();
    const ShaderStageInterface& fragment = *bundle.program.FindStage(EShaderStage::Fragment);
    const ShaderBlob& blob = *bundle.FindBlob(EShaderStage::Fragment, EShaderFormat::SPIRV);

    const SDL_GPUShaderCreateInfo info = ShaderLibrary::MakeShaderCreateInfo(fragment, blob);
    EXPECT_EQ(info.code, blob.code.Data());
    EXPECT_EQ(info.code_size, blob.code.Len());
    EXPECT_STREQ(info.entrypoint, "PSMain");
    EXPECT_EQ(info.format, SDL_GPU_SHADERFORMAT_SPIRV);
    EXPECT_EQ(info.stage, SDL_GPU_SHADERSTAGE_FRAGMENT);
    EXPECT_EQ(info.num_samplers, 5u);
    EXPECT_EQ(info.num_storage_textures, 1u);
    EXPECT_EQ(info.num_storage_buffers, 3u);
    EXPECT_EQ(info.num_uniform_buffers, 2u);
}

TEST(ShaderLibraryTest, ComputeCreateInfoUsesBundleContract)
{
    const ShaderStageInterface compute{
        .stage = EShaderStage::Compute,
        .counts = {
            .samplers = 1,
            .storage_textures = 2,
            .storage_buffers = 3,
            .readwrite_storage_textures = 4,
            .readwrite_storage_buffers = 5,
            .uniform_buffers = 6,
        },
        .thread_count = { .x = 8, .y = 4, .z = 1 },
    };
    const ShaderBlob blob{ .stage = EShaderStage::Compute, .format = EShaderFormat::DXIL, .entry_point = "CSMain", .code = { 'D', 'X', 'B', 'C' } };

    const SDL_GPUComputePipelineCreateInfo info = ShaderLibrary::MakeComputePipelineCreateInfo(compute, blob);
    EXPECT_STREQ(info.entrypoint, "CSMain");
    EXPECT_EQ(info.format, SDL_GPU_SHADERFORMAT_DXIL);
    EXPECT_EQ(info.num_samplers, 1u);
    EXPECT_EQ(info.num_readonly_storage_textures, 2u);
    EXPECT_EQ(info.num_readonly_storage_buffers, 3u);
    EXPECT_EQ(info.num_readwrite_storage_textures, 4u);
    EXPECT_EQ(info.num_readwrite_storage_buffers, 5u);
    EXPECT_EQ(info.num_uniform_buffers, 6u);
    EXPECT_EQ(info.threadcount_x, 8u);
    EXPECT_EQ(info.threadcount_y, 4u);
    EXPECT_EQ(info.threadcount_z, 1u);
}
