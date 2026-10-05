#include "gtest/gtest.h"

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"
#include "SimpleEditor/ShaderCook/ShaderCooker.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Serialization/JsonArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"

#include "SDL3/SDL_stdinc.h"

#include <string>

#if SE_HAS_HLSL_COMPILER

using namespace se;
using namespace se::editor;

// 엔진 셰이더를 쿡한 번들의 인터페이스, 진입점, 의존 파일을 골든 파일과 비교합니다.
// 셰이더를 고쳐 인터페이스가 바뀌었으면 SE_UPDATE_GOLDEN=1로 실행해 골든 파일을 다시 씁니다.
namespace
{
struct GoldenCase
{
    const char* shader;
    const char* golden_file;
};

constexpr GoldenCase GOLDEN_CASES[] = {
    { .shader = "CoreShader://DebugLine.hlsl", .golden_file = "DebugLine.json" },
    { .shader = "CoreShader://Default.hlsl", .golden_file = "Default.json" },
    { .shader = "EditorShader://Gizmo.hlsl", .golden_file = "Gizmo.json" },
    { .shader = "EditorShader://GizmoPick.hlsl", .golden_file = "GizmoPick.json" },
    { .shader = "EditorShader://WorldGrid.hlsl", .golden_file = "WorldGrid.json" },
};

[[nodiscard]] const ShaderCompiler& GetCompiler()
{
    static const ShaderCompiler compiler;
    return compiler;
}

[[nodiscard]] Path GoldenDirectory()
{
    return Path(SE_TEST_SOURCE_DIR) / "EngineTest/Golden/Shaders";
}

[[nodiscard]] bool ShouldUpdateGolden()
{
    const char* value = SDL_getenv("SE_UPDATE_GOLDEN");
    return value != nullptr && StringView{ value } == "1";
}

/** 바이트코드를 비운 번들의 JSON. 바이트코드는 도구 버전마다 바뀌므로 비교하지 않습니다. */
[[nodiscard]] String DumpBundle(ShaderBundle bundle)
{
    for (ShaderBlob& blob : bundle.blobs)
    {
        blob.code.Clear();
    }

    JsonWriter writer;
    const auto written = serde::Serialize(writer, bundle);
    EXPECT_TRUE(written.HasValue()) << (written.HasError() ? written.Error().message.CStr() : "");

    const auto text = writer.ToText();
    EXPECT_TRUE(text.HasValue()) << (text.HasError() ? text.Error().CStr() : "");
    return text.HasValue() ? text.Value() : String{};
}

/** 줄바꿈을 LF로 맞춥니다. git 체크아웃 설정에 따라 골든 파일이 CRLF일 수 있습니다. */
[[nodiscard]] String NormalizeNewlines(const String& text)
{
    std::string normalized;
    for (const char byte : text.Bytes())
    {
        if (byte != '\r')
        {
            normalized.push_back(byte);
        }
    }
    return String{ normalized.c_str(), normalized.length() };
}
} // namespace

class ShaderGoldenTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        VFS::Get().Mount("CoreShader", Path(SE_TEST_SOURCE_DIR) / "EngineCore/Shaders");
        VFS::Get().Mount("EditorShader", Path(SE_TEST_SOURCE_DIR) / "Editor/Shaders");
    }

    virtual void TearDown() override
    {
        VFS::Get().Unmount("EditorShader");
        VFS::Get().Unmount("CoreShader");
    }
};

TEST_F(ShaderGoldenTest, CookedInterfacesMatchGoldenFiles)
{
    const ShaderCooker cooker{ GetCompiler() };
    const bool update_golden = ShouldUpdateGolden();

    for (const GoldenCase& golden : GOLDEN_CASES)
    {
        SCOPED_TRACE(golden.shader);

        const auto bundle = cooker.CookFile(golden.shader);
        if (!bundle)
        {
            ADD_FAILURE() << bundle.Error().What();
            continue;
        }

        const String actual = DumpBundle(*bundle);
        const Path golden_path = GoldenDirectory() / golden.golden_file;
        if (update_golden)
        {
            fs::CreateDirectories(GoldenDirectory());
            EXPECT_TRUE(fs::WriteString(golden_path, actual));
            continue;
        }

        const auto expected = fs::ReadToString(golden_path);
        if (!expected)
        {
            ADD_FAILURE() << "Missing golden file, run with SE_UPDATE_GOLDEN=1: " << golden_path.ToString().CStr();
            continue;
        }
        EXPECT_STREQ(NormalizeNewlines(expected.Value()).CStr(), actual.CStr());
    }
}

#endif
