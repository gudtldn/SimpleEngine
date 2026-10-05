#include "gtest/gtest.h"

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"

using namespace se;
using namespace se::editor;

// 검증 규칙마다 위반하는 입력에서 문제를 보고하는지, 올바른 셰이더에서는 문제가 없는지 검증
namespace
{
/** gtest 실패 메시지에 붙일 문제 목록 */
[[nodiscard]] String Describe(const Array<String>& problems)
{
    String text;
    for (const String& problem : problems)
    {
        text += "\n  ";
        text += problem;
    }
    return text;
}

/** problems 중 하나라도 'name'을 언급하는지 여부 */
[[nodiscard]] bool Mentions(const Array<String>& problems, const char* name)
{
    const String quoted = String::Format("'{}'", name);
    return problems.FindBy([&quoted](const String& problem) { return problem.Contains(quoted); }).HasValue();
}

#if SE_HAS_HLSL_COMPILER
/** 모든 테스트가 공유하는 컴파일러. 전역 세션 생성 비용이 커서 한 번만 만듭니다. */
[[nodiscard]] const ShaderCompiler& GetCompiler()
{
    static const ShaderCompiler compiler;
    return compiler;
}

[[nodiscard]] ShaderCookResult<CompiledShaderProgram> CompileFixture(const char* file_name)
{
    const Path fixture_dir = Path(SE_TEST_SOURCE_DIR) / "EngineTest/Shaders";
    return GetCompiler().Compile({ .source_path = fixture_dir / file_name, .include_dirs = { fixture_dir } });
}
#endif
} // namespace

#if SE_HAS_HLSL_COMPILER
TEST(ShaderValidatorTest, ValidProgramsHaveNoProblems)
{
    for (const char* file_name : { "Fixture_TwoStages.hlsl", "Fixture_Storage.hlsl", "Fixture_Compute.hlsl" })
    {
        const auto result = CompileFixture(file_name);
        ASSERT_TRUE(result.HasValue()) << file_name << ": " << result.Error().What();

        const Array<String> problems = result->Validate();
        EXPECT_TRUE(problems.IsEmpty()) << file_name << Describe(problems).CStr();
    }
}

TEST(ShaderValidatorTest, ReportsResourcesOutsideStageSets)
{
    const auto result = CompileFixture("Invalid_Spaces.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const Array<String> problems = result->Validate();
    EXPECT_EQ(problems.Len(), 3u) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "albedo_texture")) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "output_texture")) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "stray_buffer")) << Describe(problems).CStr();
}

TEST(ShaderValidatorTest, ReportsStorageInsideEarlierRange)
{
    const auto result = CompileFixture("Invalid_StorageOverlap.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const Array<String> problems = result->Validate();
    EXPECT_EQ(problems.Len(), 1u) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "lights")) << Describe(problems).CStr();
}

TEST(ShaderValidatorTest, ReportsMismatchedVaryings)
{
    const auto result = CompileFixture("Invalid_Varyings.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    // tex_coord는 타입, color는 location, tangent는 semantic이 어긋납니다.
    const Array<String> problems = result->Validate();
    EXPECT_EQ(problems.Len(), 3u) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "tex_coord")) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "color")) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "tangent")) << Describe(problems).CStr();
}

TEST(ShaderValidatorTest, ReportsNonTexcoordVertexInput)
{
    const auto result = CompileFixture("Invalid_VertexSemantic.hlsl");
    ASSERT_TRUE(result.HasValue()) << result.Error().What();

    const Array<String> problems = result->Validate();
    EXPECT_EQ(problems.Len(), 1u) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "position")) << Describe(problems).CStr();
}
#endif

TEST(ShaderValidatorTest, ReportsCountsOverSdlLimits)
{
    CompiledShaderProgram program;
    program.stages.Push({
        .stage = EShaderStage::Fragment,
        .stage_interface = {
            .stage = EShaderStage::Fragment,
            .counts = { .samplers = 17, .storage_buffers = 8, .uniform_buffers = 5 },
        },
    });

    // 샘플러와 UBO만 한계를 넘고, 스토리지 버퍼 8개는 한계와 같아 허용됩니다.
    const Array<String> problems = program.Validate();
    EXPECT_EQ(problems.Len(), 2u) << Describe(problems).CStr();
}

TEST(ShaderValidatorTest, ReportsUniformOffsetMismatch)
{
    const ShaderUniformBuffer spirv{
        .name = "Material",
        .size = 32,
        .members = {
            { .name = "base_color", .offset = 0, .size = 16, .type = EShaderValueType::Float4 },
            { .name = "metallic", .offset = 16, .size = 4, .type = EShaderValueType::Float },
        },
    };
    ShaderUniformBuffer dxil = spirv;
    dxil.members[1].offset = 20;

    CompiledShaderProgram program;
    program.stages.Push({
        .stage = EShaderStage::Fragment,
        .stage_interface = { .stage = EShaderStage::Fragment, .uniform_buffers = { spirv } },
        .dxil_uniform_buffers = { dxil },
    });

    const Array<String> problems = program.Validate();
    EXPECT_EQ(problems.Len(), 1u) << Describe(problems).CStr();
    EXPECT_TRUE(Mentions(problems, "metallic")) << Describe(problems).CStr();
}
