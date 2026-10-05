#include "gtest/gtest.h"

#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Shader/ShaderBundleSource.h"

#include "SDL3/SDL_filesystem.h"

#include <memory>

using namespace se;

// DDC 번들 소스의 키 계산, 정상 읽기, 없는 번들·버전 불일치·손상 거부 검증
namespace
{
constexpr StringView PROGRAM = "CoreShader://DebugLine.hlsl";

[[nodiscard]] ShaderBundle MakeBundle()
{
    ShaderBundle bundle;
    bundle.program.stages.Push({ .stage = EShaderStage::Vertex });
    bundle.blobs.Push({ .stage = EShaderStage::Vertex, .format = EShaderFormat::SPIRV, .entry_point = "VSMain", .code = { 0x03, 0x02, 0x23, 0x07 } });
    bundle.dependencies = { String{ PROGRAM } };
    return bundle;
}
} // namespace

class ShaderBundleSourceTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        char* pref = SDL_GetPrefPath("SimpleEngine", "Tests");
        root = Path(pref) / Path("ShaderBundleSourceTest");
        SDL_free(pref);

        fs::RemoveAll(root);
        ddc = std::make_unique<DerivedDataCache>(root);
    }

    virtual void TearDown() override
    {
        ddc.reset();
        fs::RemoveAll(root);
    }

    void Store(u32 cache_version, Array<u8> payload) const
    {
        ASSERT_TRUE(ddc->Store(DdcShaderBundleSource::KeyOf(PROGRAM), CacheEntry{ .cache_version = cache_version, .payload = std::move(payload) }));
    }

    Path root;
    std::unique_ptr<DerivedDataCache> ddc;
};

TEST_F(ShaderBundleSourceTest, KeyOfKeepsExistingCacheKeys)
{
    // 키가 바뀌면 이미 쿡해 둔 DDC 번들을 모두 다시 쿡해야 하므로 값을 고정합니다.
    EXPECT_STREQ(DdcShaderBundleSource::KeyOf(PROGRAM).ToString().CStr(), "c5301c06-2916-8599-b7ce-3e92e7f6ba0a");
}

TEST_F(ShaderBundleSourceTest, LoadsStoredBundle)
{
    const ShaderBundle expected = MakeBundle();
    Store(ShaderBundle::FORMAT_VERSION, expected.Serialize());

    const auto bundle = DdcShaderBundleSource{ *ddc }.Load(PROGRAM);
    ASSERT_TRUE(bundle.HasValue()) << bundle.Error().CStr();
    EXPECT_TRUE(*bundle == expected);
}

TEST_F(ShaderBundleSourceTest, RejectsMissingBundle)
{
    const auto bundle = DdcShaderBundleSource{ *ddc }.Load(PROGRAM);
    EXPECT_FALSE(bundle.HasValue());
}

TEST_F(ShaderBundleSourceTest, RejectsOtherFormatVersion)
{
    Store(ShaderBundle::FORMAT_VERSION + 1, MakeBundle().Serialize());

    const auto bundle = DdcShaderBundleSource{ *ddc }.Load(PROGRAM);
    EXPECT_FALSE(bundle.HasValue());
}

TEST_F(ShaderBundleSourceTest, RejectsCorruptedPayload)
{
    Store(ShaderBundle::FORMAT_VERSION, { 0xDE, 0xAD, 0xBE, 0xEF });

    const auto bundle = DdcShaderBundleSource{ *ddc }.Load(PROGRAM);
    EXPECT_FALSE(bundle.HasValue());
}
