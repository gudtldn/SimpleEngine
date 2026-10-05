#include "gtest/gtest.h"

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"
#include "SimpleEditor/ShaderCook/ShaderCooker.h"

#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Shader/ShaderBundleSource.h"

#include "SDL3/SDL_filesystem.h"

#if SE_HAS_HLSL_COMPILER

using namespace se;
using namespace se::editor;

// 쿠커의 DDC 저장, 캐시 적중, include 변경 감지, 검증 실패 처리 검증
namespace
{
constexpr StringView COOK_DIR = "CookTest://";

constexpr StringView COMMON_HEADER = "float4 Tint() { return float4(1.0f, 1.0f, 1.0f, 1.0f); }\n";

constexpr StringView PROGRAM_SOURCE = R"(#include "Common.hlsli"

[shader("vertex")]
float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return float4(float(vertex_id), 0.0f, 0.0f, 1.0f);
}

[shader("pixel")]
float4 PSMain(float4 position : SV_Position) : SV_Target0
{
    return Tint();
}
)";

/** 모든 테스트가 공유하는 컴파일러. 전역 세션 생성 비용이 커서 한 번만 만듭니다. */
[[nodiscard]] const ShaderCompiler& GetCompiler()
{
    static const ShaderCompiler compiler;
    return compiler;
}
} // namespace

class ShaderCookerTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        char* pref = SDL_GetPrefPath("SimpleEngine", "Tests");
        root = Path(pref) / Path("ShaderCookerTest");
        SDL_free(pref);

        fs::RemoveAll(root);
        fs::CreateDirectories(ShaderDirectory());

        // 쿠커는 공통 헤더 경로로 CoreShader를 항상 넘깁니다.
        VFS::Get().Mount("CoreShader", Path(SE_TEST_SOURCE_DIR) / "EngineCore/Shaders");
        VFS::Get().Mount("CookTest", ShaderDirectory());
    }

    virtual void TearDown() override
    {
        VFS::Get().Unmount("CookTest");
        VFS::Get().Unmount("CoreShader");
        fs::RemoveAll(root);
    }

    [[nodiscard]] Path ShaderDirectory() const { return root / Path("Shaders"); }

    void WriteShader(StringView file_name, StringView content) const
    {
        ASSERT_TRUE(fs::WriteString(ShaderDirectory() / Path(file_name), content));
    }

    Path root;
};

TEST_F(ShaderCookerTest, StoresBundleAndSkipsUpToDateShader)
{
    WriteShader("Common.hlsli", COMMON_HEADER);
    WriteShader("Program.hlsl", PROGRAM_SOURCE);
    DerivedDataCache ddc{ root / Path("DDC") };
    const ShaderCooker cooker{ GetCompiler() };

    const ShaderCookSummary first = cooker.CookDirectory(COOK_DIR, ddc);
    EXPECT_EQ(first.cooked, 1u);
    EXPECT_EQ(first.failed, 0u);

    // 쿠커가 쓴 번들을 런타임이 읽는 경로 그대로 읽습니다.
    const auto bundle = DdcShaderBundleSource{ ddc }.Load("CookTest://Program.hlsl");
    ASSERT_TRUE(bundle.HasValue()) << bundle.Error().CStr();
    ASSERT_EQ(bundle->dependencies.Len(), 2u);
    EXPECT_STREQ(bundle->dependencies[0].CStr(), "CookTest://Common.hlsli");
    EXPECT_STREQ(bundle->dependencies[1].CStr(), "CookTest://Program.hlsl");
    EXPECT_EQ(bundle->program.stages.Len(), 2u);

    const ShaderCookSummary second = cooker.CookDirectory(COOK_DIR, ddc);
    EXPECT_EQ(second.cooked, 0u);
    EXPECT_EQ(second.up_to_date, 1u);
}

TEST_F(ShaderCookerTest, RecooksWhenIncludedFileChanges)
{
    WriteShader("Common.hlsli", COMMON_HEADER);
    WriteShader("Program.hlsl", PROGRAM_SOURCE);
    DerivedDataCache ddc{ root / Path("DDC") };
    const ShaderCooker cooker{ GetCompiler() };

    ASSERT_EQ(cooker.CookDirectory(COOK_DIR, ddc).cooked, 1u);

    WriteShader("Common.hlsli", "float4 Tint() { return float4(0.5f, 0.5f, 0.5f, 1.0f); }\n");

    const ShaderCookSummary second = cooker.CookDirectory(COOK_DIR, ddc);
    EXPECT_EQ(second.cooked, 1u);
    EXPECT_EQ(second.up_to_date, 0u);
}

TEST_F(ShaderCookerTest, ReportsValidationFailure)
{
    const auto fixture = fs::ReadToString(Path(SE_TEST_SOURCE_DIR) / "EngineTest/Shaders/Invalid_Varyings.hlsl");
    ASSERT_TRUE(fixture.HasValue());
    WriteShader("Invalid_Varyings.hlsl", fixture.Value());

    const ShaderCooker cooker{ GetCompiler() };
    const auto result = cooker.CookFile("CookTest://Invalid_Varyings.hlsl");

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().GetType(), ShaderCookError::ValidationFailed);
}

#endif
