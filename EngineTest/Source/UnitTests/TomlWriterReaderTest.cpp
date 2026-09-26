#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"

#include <bit>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

using namespace se;

// TomlWriter/TomlReader의 노드 규칙(정수 범위, f32 표현, enum 이름), 레거시 설정 파일 호환, 오류 경로 검증
namespace se_toml_test
{
/** 이름을 등록한 enum. 설정 파일의 present_mode를 흉내 냅니다. */
enum class EPresentMode : u8
{
    Mailbox,
    VSync,
    Immediate,
};

/** [window] 섹션을 흉내 내는 타입 */
struct WindowSettings
{
    String title = "SimpleEngine";
    u32 width = 1280;
    u32 height = 720;
    bool fullscreen = false;

    [[nodiscard]] bool operator==(const WindowSettings&) const = default;
};

/** [graphics] 섹션을 흉내 내는 타입 */
struct GraphicsSettings
{
    EPresentMode present_mode = EPresentMode::Mailbox;

    [[nodiscard]] bool operator==(const GraphicsSettings&) const = default;
};

/** [performance] 섹션을 흉내 내는 타입 */
struct PerformanceSettings
{
    u32 target_fps = 240;
    f32 busy_wait_ratio = 0.1f;

    [[nodiscard]] bool operator==(const PerformanceSettings&) const = default;
};

/** 설정 파일 하나를 흉내 내는 루트 타입. 중첩 struct, 문자열 배열, enum, f32를 담습니다. */
struct EditorConfig
{
    WindowSettings window;
    GraphicsSettings graphics;
    PerformanceSettings performance;
    Array<String> schemes = { "CoreAssets", "EditorAssets" };

    [[nodiscard]] bool operator==(const EditorConfig&) const = default;
};

/** 정수 폭별 경계값과 범위 검사 검증용 */
struct IntFields
{
    i8 i8_value = 0;
    u8 u8_value = 0;
    i16 i16_value = 0;
    u16 u16_value = 0;
    i32 i32_value = 0;
    u32 u32_value = 0;
    i64 i64_value = 0;
    u64 u64_value = 0;

    [[nodiscard]] bool operator==(const IntFields&) const = default;
};

/** 실수 폭별 검증용 */
struct FloatFields
{
    f32 single = 0.0f;
    f64 wide = 0.0;
};

/** 배열 안 struct의 왕복과 오류 경로(items[1].value) 검증용 원소 */
struct Item
{
    i32 value = 0;

    [[nodiscard]] bool operator==(const Item&) const = default;
};

/** 배열 안 struct의 왕복과 오류 경로 검증용 */
struct HasItems
{
    Array<Item> items;
};

/** 아직 지원하지 않는 Map 노드 검증용 */
struct HasMap
{
    HashMap<String, i32> scores;
};

/** 아직 지원하지 않는 Optional 노드 검증용 */
struct HasOptional
{
    Optional<i32> value;
};

/** 아직 지원하지 않는 순서 없는 시퀀스 검증용 */
struct HasSet
{
    HashSet<i32> tags;
};
} // namespace se_toml_test

SE_DECLARE_REFLECTION(se_toml_test::EPresentMode)
SE_DECLARE_REFLECTION(se_toml_test::WindowSettings)
SE_DECLARE_REFLECTION(se_toml_test::GraphicsSettings)
SE_DECLARE_REFLECTION(se_toml_test::PerformanceSettings)
SE_DECLARE_REFLECTION(se_toml_test::EditorConfig)
SE_DECLARE_REFLECTION(se_toml_test::IntFields)
SE_DECLARE_REFLECTION(se_toml_test::FloatFields)
SE_DECLARE_REFLECTION(se_toml_test::Item)
SE_DECLARE_REFLECTION(se_toml_test::HasItems)
SE_DECLARE_REFLECTION(se_toml_test::HasMap)
SE_DECLARE_REFLECTION(se_toml_test::HasOptional)
SE_DECLARE_REFLECTION(se_toml_test::HasSet)

SE_REFLECT_ENUM_BEGIN(se_toml_test::EPresentMode)
    SE_ENUM_VALUE(Mailbox)
    SE_ENUM_VALUE(VSync)
    SE_ENUM_VALUE(Immediate)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se_toml_test::WindowSettings)
    SE_FIELD(title)
    SE_FIELD(width)
    SE_FIELD(height)
    SE_FIELD(fullscreen)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::GraphicsSettings)
    SE_FIELD(present_mode)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::PerformanceSettings)
    SE_FIELD(target_fps)
    SE_FIELD(busy_wait_ratio)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::EditorConfig)
    SE_FIELD(window)
    SE_FIELD(graphics)
    SE_FIELD(performance)
    SE_FIELD(schemes)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::IntFields)
    SE_FIELD(i8_value)
    SE_FIELD(u8_value)
    SE_FIELD(i16_value)
    SE_FIELD(u16_value)
    SE_FIELD(i32_value)
    SE_FIELD(u32_value)
    SE_FIELD(i64_value)
    SE_FIELD(u64_value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::FloatFields)
    SE_FIELD(single)
    SE_FIELD(wide)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::Item)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasItems)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasMap)
    SE_FIELD(scores)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasOptional)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasSet)
    SE_FIELD(tags)
SE_REFLECT_END()


namespace
{
/** value를 새 테이블에 쓰고 돌려줍니다. 쓰기에 실패하면 테스트를 실패시킵니다. */
template <typename T>
toml::table WriteToTable(const T& value)
{
    toml::table table;
    TomlWriter writer(table);
    const auto written = serde::Serialize(writer, value);
    EXPECT_TRUE(written.HasValue()) << (written.HasError() ? written.Error().message.CStr() : "");
    return table;
}

/** TOML 텍스트를 테이블로 파싱합니다. 문법 오류면 테스트를 실패시키고 빈 테이블을 돌려줍니다. */
toml::table ParseToml(std::string_view text)
{
    toml::parse_result result = toml::parse(text);
    if (!result)
    {
        ADD_FAILURE() << "TOML parse error: " << result.error().description();
        return {};
    }
    return std::move(result).table();
}

/** 테이블을 toml++ 기본 형식의 텍스트로 만듭니다. */
std::string ToText(const toml::table& table)
{
    std::ostringstream stream;
    stream << table;
    return stream.str();
}
} // namespace


// --- 왕복 ---

TEST(TomlWriterReaderTest, SettingsRoundTrip)
{
    using namespace se_toml_test;

    const EditorConfig original{
        .window = WindowSettings{ .title = "Editor", .width = 1600, .height = 900, .fullscreen = true },
        .graphics = GraphicsSettings{ .present_mode = EPresentMode::VSync },
        .performance = PerformanceSettings{ .target_fps = 144, .busy_wait_ratio = 0.25f },
        .schemes = { "CoreAssets", "EditorAssets", "GameAssets" },
    };
    const toml::table table = WriteToTable(original);

    TomlReader reader(table);
    EditorConfig result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
}

TEST(TomlWriterReaderTest, WritesTablesKeysAndArrays)
{
    const toml::table table = WriteToTable(se_toml_test::EditorConfig{});

    EXPECT_EQ(table["window"]["title"].value_exact<std::string>(), "SimpleEngine");
    EXPECT_EQ(table["window"]["width"].value_exact<i64>(), 1280);
    EXPECT_EQ(table["window"]["fullscreen"].value_exact<bool>(), false);
    EXPECT_EQ(table["graphics"]["present_mode"].value_exact<std::string>(), "Mailbox");
    EXPECT_EQ(table["performance"]["busy_wait_ratio"].value_exact<f64>(), 0.1);

    ASSERT_TRUE(table["schemes"].is_array());
    EXPECT_EQ(table["schemes"].as_array()->size(), 2u);
    EXPECT_EQ(table["schemes"][0].value_exact<std::string>(), "CoreAssets");
}

TEST(TomlWriterReaderTest, ArrayOfStructsRoundTrip)
{
    using namespace se_toml_test;

    HasItems original;
    original.items.Push(Item{ .value = 1 });
    original.items.Push(Item{ .value = 2 });
    const toml::table table = WriteToTable(original);

    TomlReader reader(table);
    HasItems result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result.items, original.items);
}


// --- 정수 ---

TEST(TomlWriterReaderTest, IntBoundariesRoundTrip)
{
    using namespace se_toml_test;

    const IntFields minimums{
        .i8_value = std::numeric_limits<i8>::min(),
        .u8_value = std::numeric_limits<u8>::min(),
        .i16_value = std::numeric_limits<i16>::min(),
        .u16_value = std::numeric_limits<u16>::min(),
        .i32_value = std::numeric_limits<i32>::min(),
        .u32_value = std::numeric_limits<u32>::min(),
        .i64_value = std::numeric_limits<i64>::min(),
        .u64_value = std::numeric_limits<u64>::min(),
    };
    const IntFields maximums{
        .i8_value = std::numeric_limits<i8>::max(),
        .u8_value = std::numeric_limits<u8>::max(),
        .i16_value = std::numeric_limits<i16>::max(),
        .u16_value = std::numeric_limits<u16>::max(),
        .i32_value = std::numeric_limits<i32>::max(),
        .u32_value = std::numeric_limits<u32>::max(),
        .i64_value = std::numeric_limits<i64>::max(),
        .u64_value = std::numeric_limits<u64>::max(),
    };

    for (const IntFields& original : { minimums, maximums })
    {
        const toml::table table = WriteToTable(original);
        TomlReader reader(table);
        IntFields result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result, original);
    }
}

TEST(TomlWriterReaderTest, U64AboveI64IsWrittenAsString)
{
    const toml::table table = WriteToTable(se_toml_test::IntFields{ .u64_value = std::numeric_limits<u64>::max() });

    // TOML 정수는 i64라서 i64를 넘는 u64만 10진 문자열로 씀
    EXPECT_EQ(table["u64_value"].value_exact<std::string>(), "18446744073709551615");
    EXPECT_EQ(table["i64_value"].value_exact<i64>(), 0);
}

TEST(TomlWriterReaderTest, IntReadErrors)
{
    struct Case
    {
        std::string_view text;
        const char* path;
        const char* message;
    };
    const Case cases[] = {
        { "u8_value = 256", "u8_value", "TomlReader: 256 is out of range for u8." },
        { "i8_value = -129", "i8_value", "TomlReader: -129 is out of range for i8." },
        { "u32_value = -1", "u32_value", "TomlReader: -1 is out of range for u32." },
        { "u64_value = -1", "u64_value", "TomlReader: -1 is out of range for u64." },
        { "i32_value = 25.0", "i32_value", "TomlReader: expected an integer, got a float." },
        { "i64_value = '5'", "i64_value", "TomlReader: expected an integer, got a string." },
        { "u64_value = 'abc'", "u64_value", "TomlReader: 'abc' is not a valid u64 number." },
    };

    for (const Case& c : cases)
    {
        const toml::table table = ParseToml(c.text);
        TomlReader reader(table);
        se_toml_test::IntFields result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError()) << c.text;
        EXPECT_EQ(read_result.Error().path, c.path) << c.text;
        EXPECT_EQ(read_result.Error().message, c.message) << c.text;
    }
}


// --- 실수 ---

TEST(TomlWriterReaderTest, F32IsWrittenAsShortestText)
{
    const toml::table table = WriteToTable(se_toml_test::PerformanceSettings{});
    const std::string text = ToText(table);

    // 레거시는 f32를 넓힌 값 그대로 써서 0.10000000149011612가 됨
    EXPECT_NE(text.find("busy_wait_ratio = 0.1\n"), std::string::npos) << text;
    EXPECT_EQ(text.find("0.10000000149011612"), std::string::npos) << text;
}

TEST(TomlWriterReaderTest, F32RoundTripsExactly)
{
    using namespace se_toml_test;

    const f32 values[] = {
        0.1f,
        1.0f / 3.0f,
        3.14159f,
        17.0f,
        -0.0f,
        std::numeric_limits<f32>::max(),
        std::numeric_limits<f32>::lowest(),
        std::numeric_limits<f32>::min(),
        std::numeric_limits<f32>::denorm_min(),
        std::numeric_limits<f32>::infinity(),
        -std::numeric_limits<f32>::infinity(),
        // 가장 짧은 표현(7.038531e-26)을 f64로 읽으면 두 f32의 정확한 중간값이 되는 값
        std::bit_cast<f32>(0x15ae43fdu),
        std::bit_cast<f32>(0x95ae43fdu),
    };

    for (const f32 value : values)
    {
        const toml::table table = WriteToTable(FloatFields{ .single = value });
        TomlReader reader(table);
        FloatFields result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue()) << value;
        EXPECT_EQ(std::bit_cast<u32>(result.single), std::bit_cast<u32>(value)) << value;
    }
}

TEST(TomlWriterReaderTest, F32DoubleRoundingValueIsWrittenWidened)
{
    // 짧은 표현이 이웃 f32로 반올림되는 값은 넓힌 값을 그대로 씀
    const f32 value = std::bit_cast<f32>(0x15ae43fdu);
    const toml::table table = WriteToTable(se_toml_test::FloatFields{ .single = value });

    EXPECT_EQ(table["single"].value_exact<f64>(), static_cast<f64>(value));
}

TEST(TomlWriterReaderTest, F32NanRoundTrips)
{
    const toml::table table = WriteToTable(se_toml_test::FloatFields{ .single = std::numeric_limits<f32>::quiet_NaN() });

    TomlReader reader(table);
    se_toml_test::FloatFields result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_TRUE(std::isnan(result.single));
}

TEST(TomlWriterReaderTest, FloatReadRules)
{
    using namespace se_toml_test;

    {
        // 사람이 정수로 적은 실수도 받음
        const toml::table table = ParseToml("single = 17\nwide = 2");
        TomlReader reader(table);
        FloatFields result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result.single, 17.0f);
        EXPECT_EQ(result.wide, 2.0);
    }
    {
        // FLT_MAX의 가장 짧은 표현은 FLT_MAX보다 조금 크지만 반올림하면 FLT_MAX
        const toml::table table = ParseToml("single = 3.4028235e+38");
        TomlReader reader(table);
        FloatFields result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result.single, std::numeric_limits<f32>::max());
    }
    {
        const toml::table table = ParseToml("single = 1e39");
        TomlReader reader(table);
        FloatFields result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().path, "single");
        EXPECT_EQ(read_result.Error().message, "TomlReader: 1e+39 is out of range for f32.");
    }
    {
        const toml::table table = ParseToml("wide = true");
        TomlReader reader(table);
        FloatFields result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().message, "TomlReader: expected a float, got a boolean.");
    }
}


// --- enum ---

TEST(TomlWriterReaderTest, EnumIsWrittenByName)
{
    using namespace se_toml_test;

    const toml::table named = WriteToTable(GraphicsSettings{ .present_mode = EPresentMode::Immediate });
    EXPECT_EQ(named["present_mode"].value_exact<std::string>(), "Immediate");

    // 이름이 없는 값은 정수로 씀
    const toml::table unnamed = WriteToTable(GraphicsSettings{ .present_mode = static_cast<EPresentMode>(7) });
    EXPECT_EQ(unnamed["present_mode"].value_exact<i64>(), 7);

    TomlReader reader(unnamed);
    GraphicsSettings result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result.present_mode, static_cast<EPresentMode>(7));
}

TEST(TomlWriterReaderTest, EnumIsReadByNameOrInteger)
{
    using namespace se_toml_test;

    {
        const toml::table table = ParseToml("present_mode = 'VSync'");
        TomlReader reader(table);
        GraphicsSettings result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result.present_mode, EPresentMode::VSync);
    }
    {
        // 레거시는 enum을 정수로 씀
        const toml::table table = ParseToml("present_mode = 2");
        TomlReader reader(table);
        GraphicsSettings result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result.present_mode, EPresentMode::Immediate);
    }
    {
        const toml::table table = ParseToml("present_mode = 'Fast'");
        TomlReader reader(table);
        GraphicsSettings result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().path, "present_mode");
        EXPECT_EQ(read_result.Error().message, "TomlReader: 'Fast' is not a name of this enum.");
    }
}

TEST(TomlWriterReaderTest, UnnamedEnumAboveI64IsError)
{
    // 문자열로 쓰면 이름으로 읽혀 되읽을 수 없으므로 쓰기에서 막음
    toml::table table;
    TomlWriter writer(table);
    writer.BeginStruct();
    writer.Field("flag");
    writer.Enum(-1, EIntWidth::Bits64, false, {});

    ASSERT_TRUE(writer.HasError());
    EXPECT_EQ(String(writer.GetError()), "TomlWriter: enum value 18446744073709551615 has no name and does not fit in a TOML integer.");
}


// --- 레거시 호환과 필드 누락 ---

TEST(TomlWriterReaderTest, ReadsLegacyConfigText)
{
    using namespace se_toml_test;

    // 레거시 TomlWriter_v1이 쓴 설정 파일의 모양: f32는 넓힌 값, enum은 정수, 모르는 키와 섹션이 있음
    const toml::table table = ParseToml(R"(
[graphics]
present_mode = 0

[performance]
busy_wait_ratio = 0.10000000149011612
target_fps = 240

[vfs]
CoreAssets = 'EngineCore/Assets'

[window]
borderless = false
fullscreen = false
height = 900
resizable = true
title = 'SimpleEngine'
width = 1600
)");

    TomlReader reader(table);
    EditorConfig result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result.graphics.present_mode, EPresentMode::Mailbox);
    EXPECT_EQ(result.performance.busy_wait_ratio, 0.1f);
    EXPECT_EQ(result.performance.target_fps, 240u);
    EXPECT_EQ(result.window.width, 1600u);
    EXPECT_EQ(result.window.height, 900u);

    // 파일에 없는 필드는 현재 값(기본값)을 유지
    EXPECT_EQ(result.schemes, (Array<String>{ "CoreAssets", "EditorAssets" }));

    // 이 타입에 없는 키와 섹션은 경고로 남음 (window를 먼저 닫으므로 window의 키가 먼저)
    const ArrayView<const String> warnings = reader.GetWarnings();
    ASSERT_EQ(warnings.Len(), 3u);
    EXPECT_EQ(warnings[0], "TomlReader: unknown key 'window.borderless' is ignored.");
    EXPECT_EQ(warnings[1], "TomlReader: unknown key 'window.resizable' is ignored.");
    EXPECT_EQ(warnings[2], "TomlReader: unknown key 'vfs' is ignored.");
}


// --- 경고 ---

TEST(TomlWriterReaderTest, UnknownKeysAreWarnings)
{
    using namespace se_toml_test;

    const toml::table table = ParseToml(R"(
typo_at_root = 1

[window]
widht = 900
width = 1600
)");

    TomlReader reader(table);
    EditorConfig result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

    // 경고가 있어도 아는 키는 읽고, 오타 난 키의 필드는 기본값으로 남음
    EXPECT_EQ(result.window.width, 1600u);
    EXPECT_EQ(result.window.height, 720u);

    const ArrayView<const String> warnings = reader.GetWarnings();
    ASSERT_EQ(warnings.Len(), 2u);
    EXPECT_EQ(warnings[0], "TomlReader: unknown key 'window.widht' is ignored.");
    EXPECT_EQ(warnings[1], "TomlReader: unknown key 'typo_at_root' is ignored.");
}

TEST(TomlWriterReaderTest, UnknownKeyInArrayElementHasIndexedPath)
{
    const toml::table table = ParseToml("items = [ { value = 1 }, { value = 2, extra = true } ]");

    TomlReader reader(table);
    se_toml_test::HasItems result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

    const ArrayView<const String> warnings = reader.GetWarnings();
    ASSERT_EQ(warnings.Len(), 1u);
    EXPECT_EQ(warnings[0], "TomlReader: unknown key 'items[1].extra' is ignored.");
}

TEST(TomlWriterReaderTest, NoWarningsForWrittenTable)
{
    // TomlWriter가 쓴 테이블에는 타입에 없는 키가 없음
    const toml::table table = WriteToTable(se_toml_test::EditorConfig{});

    TomlReader reader(table);
    se_toml_test::EditorConfig result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
}


// --- 오류 ---

TEST(TomlWriterReaderTest, TypeMismatchReportsPath)
{
    {
        const toml::table table = ParseToml("[window]\nwidth = 'wide'");
        TomlReader reader(table);
        se_toml_test::EditorConfig result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().path, "window.width");
        EXPECT_EQ(read_result.Error().message, "TomlReader: expected an integer, got a string.");
    }
    {
        const toml::table table = ParseToml("window = 3");
        TomlReader reader(table);
        se_toml_test::EditorConfig result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().path, "window");
        EXPECT_EQ(read_result.Error().message, "TomlReader: expected a table, got an integer.");
    }
    {
        const toml::table table = ParseToml("items = [ { value = 1 }, { value = 'x' } ]");
        TomlReader reader(table);
        se_toml_test::HasItems result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().path, "items[1].value");
    }
}

TEST(TomlWriterReaderTest, RootMustBeStruct)
{
    toml::table table;
    TomlWriter writer(table);
    const auto write_result = serde::Serialize(writer, i32{ 42 });
    ASSERT_TRUE(write_result.HasError());
    EXPECT_EQ(write_result.Error().message, "TomlWriter: the root value must be a single struct because a TOML document is a table.");

    TomlReader reader(table);
    i32 value = 0;
    const auto read_result = serde::Deserialize(reader, value);
    ASSERT_TRUE(read_result.HasError());
    EXPECT_EQ(read_result.Error().message, "TomlReader: the root value must be a single struct because a TOML document is a table.");
}

TEST(TomlWriterReaderTest, NodesNotSupportedYetAreErrors)
{
    using namespace se_toml_test;

    const auto write_error = [](const auto& value) -> String
    {
        toml::table table;
        TomlWriter writer(table);
        const auto write_result = serde::Serialize(writer, value);
        return write_result.HasError() ? write_result.Error().message : String("no error");
    };
    EXPECT_EQ(write_error(HasMap{}), "TomlWriter: maps are not supported yet.");
    EXPECT_EQ(write_error(HasOptional{}), "TomlWriter: optional values are not supported yet.");
    EXPECT_EQ(write_error(HasSet{}), "TomlWriter: unordered sequences are not supported yet.");

    const toml::table table = ParseToml("scores = {}\nvalue = 1");
    {
        TomlReader reader(table);
        HasMap result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().message, "TomlReader: maps are not supported yet.");
    }
    {
        TomlReader reader(table);
        HasOptional result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().message, "TomlReader: optional values are not supported yet.");
    }
}

TEST(TomlWriterReaderTest, NodeOrderMistakesAreErrors)
{
    {
        // Field 없이 값을 씀
        toml::table table;
        TomlWriter writer(table);
        writer.BeginStruct();
        writer.Int(1, EIntWidth::Bits32, true);
        EXPECT_EQ(String(writer.GetError()), "TomlWriter: a value inside a struct needs a Field name first.");
    }
    {
        // 값 없이 다음 Field를 씀
        toml::table table;
        TomlWriter writer(table);
        writer.BeginStruct();
        writer.Field("first");
        writer.Field("second");
        EXPECT_EQ(String(writer.GetError()), "TomlWriter: field 'first' has no value.");
    }
    {
        // 배열 길이보다 많이 읽음
        const toml::table table = ParseToml("numbers = [ 1 ]");
        TomlReader reader(table);
        reader.BeginStruct();
        ASSERT_TRUE(reader.Field("numbers"));
        u64 count = 0;
        reader.BeginSeq(count);
        EXPECT_EQ(count, 1u);

        i64 value = 0;
        reader.Int(value, EIntWidth::Bits32, true);
        reader.Int(value, EIntWidth::Bits32, true);
        EXPECT_EQ(String(reader.GetError()), "TomlReader: read past the end of an array of length 1.");
    }
}

TEST(TomlWriterReaderTest, ErrorIsSticky)
{
    toml::table table;
    TomlWriter writer(table);
    writer.BeginStruct();
    writer.Int(1, EIntWidth::Bits32, true);

    // 오류 뒤의 노드는 테이블을 바꾸지 않음
    writer.Field("after");
    writer.Int(2, EIntWidth::Bits32, true);
    writer.EndStruct();

    EXPECT_TRUE(writer.HasError());
    EXPECT_TRUE(table.empty());
}
