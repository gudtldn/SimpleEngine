#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/JsonArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <ranges>
#include <string_view>

using namespace se;

// JsonWriter/JsonReader의 노드 규칙(정수 범위와 큰 정수 문자열, f32 표현, enum 이름, 맵, Optional, Bytes, 정렬), 경고, 파싱 오류, 출력 모양 골든 텍스트 검증
namespace se_json_test
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

/** 문자열 key 맵(객체) 검증용 */
struct HasMap
{
    HashMap<String, i32> scores;

    [[nodiscard]] bool operator==(const HasMap&) const = default;
};

/** struct 값을 담은 객체 맵에서 모르는 키의 경고 위치 검증용 */
struct HasItemMap
{
    HashMap<String, Item> items;
};

/** 정수 key 맵([key, value] 쌍 배열) 검증용 */
struct HasIdMap
{
    HashMap<i32, Item> items;

    [[nodiscard]] bool operator==(const HasIdMap&) const = default;
};

/** 이름을 등록한 enum key 맵 검증용 */
struct HasModeMap
{
    HashMap<EPresentMode, i32> modes;

    [[nodiscard]] bool operator==(const HasModeMap&) const = default;
};

/** 2^53 - 1을 넘는 u64 key 맵 검증용. 그런 key는 10진 문자열로 쓰입니다. */
struct HasU64Map
{
    HashMap<u64, i32> values;

    [[nodiscard]] bool operator==(const HasU64Map&) const = default;
};

/** Optional 필드 검증용 */
struct HasOptional
{
    Optional<i32> value;

    [[nodiscard]] bool operator==(const HasOptional&) const = default;
};

/** 기본값이 Some인 Optional 필드 검증용 */
struct HasDefaultSome
{
    Optional<i32> value = 5;
};

/** 시퀀스 원소의 None 검증용 */
struct HasOptionalArray
{
    Array<Optional<i32>> values;
};

/** 다른 Optional 안의 None 검증용 */
struct HasNestedOptional
{
    Optional<Optional<i32>> value;
};

/** 순서 없는 시퀀스의 정렬 검증용 */
struct HasSets
{
    HashSet<i32> ints;
    HashSet<f64> floats;
    HashSet<bool> flags;
    HashSet<String> names;
    HashSet<EPresentMode> modes;

    [[nodiscard]] bool operator==(const HasSets&) const = default;
};

/** 출력 모양 골든에서 스칼라만 담은 객체가 되는 벡터 */
struct Vec3
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;

    [[nodiscard]] bool operator==(const Vec3&) const = default;
};

/** 출력 모양 골든에서 객체 안의 객체를 담는 타입 */
struct Transform
{
    String name;
    Vec3 position;

    [[nodiscard]] bool operator==(const Transform&) const = default;
};

/** 출력 모양 골든에서 객체 배열을 담는 타입 */
struct HasTransforms
{
    Array<Transform> items;

    [[nodiscard]] bool operator==(const HasTransforms&) const = default;
};

/** 출력 모양 골든에서 스칼라 배열을 담는 타입 */
struct HasNumbers
{
    Array<i32> numbers;

    [[nodiscard]] bool operator==(const HasNumbers&) const = default;
};

/** 출력 모양 골든에서 빈 배열과 빈 객체를 담는 타입 */
struct HasEmpties
{
    Array<i32> numbers;
    HashMap<String, i32> scores;
};

/** 문자열 이스케이프와 비ASCII 검증용 */
struct HasText
{
    String text;

    [[nodiscard]] bool operator==(const HasText&) const = default;
};
} // namespace se_json_test

SE_DECLARE_REFLECTION(se_json_test::EPresentMode)
SE_DECLARE_REFLECTION(se_json_test::WindowSettings)
SE_DECLARE_REFLECTION(se_json_test::GraphicsSettings)
SE_DECLARE_REFLECTION(se_json_test::PerformanceSettings)
SE_DECLARE_REFLECTION(se_json_test::EditorConfig)
SE_DECLARE_REFLECTION(se_json_test::IntFields)
SE_DECLARE_REFLECTION(se_json_test::FloatFields)
SE_DECLARE_REFLECTION(se_json_test::Item)
SE_DECLARE_REFLECTION(se_json_test::HasItems)
SE_DECLARE_REFLECTION(se_json_test::HasMap)
SE_DECLARE_REFLECTION(se_json_test::HasItemMap)
SE_DECLARE_REFLECTION(se_json_test::HasIdMap)
SE_DECLARE_REFLECTION(se_json_test::HasModeMap)
SE_DECLARE_REFLECTION(se_json_test::HasU64Map)
SE_DECLARE_REFLECTION(se_json_test::HasOptional)
SE_DECLARE_REFLECTION(se_json_test::HasDefaultSome)
SE_DECLARE_REFLECTION(se_json_test::HasOptionalArray)
SE_DECLARE_REFLECTION(se_json_test::HasNestedOptional)
SE_DECLARE_REFLECTION(se_json_test::HasSets)
SE_DECLARE_REFLECTION(se_json_test::Vec3)
SE_DECLARE_REFLECTION(se_json_test::Transform)
SE_DECLARE_REFLECTION(se_json_test::HasTransforms)
SE_DECLARE_REFLECTION(se_json_test::HasNumbers)
SE_DECLARE_REFLECTION(se_json_test::HasEmpties)
SE_DECLARE_REFLECTION(se_json_test::HasText)

SE_REFLECT_ENUM_BEGIN(se_json_test::EPresentMode)
    SE_ENUM_VALUE(Mailbox)
    SE_ENUM_VALUE(VSync)
    SE_ENUM_VALUE(Immediate)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se_json_test::WindowSettings)
    SE_FIELD(title)
    SE_FIELD(width)
    SE_FIELD(height)
    SE_FIELD(fullscreen)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::GraphicsSettings)
    SE_FIELD(present_mode)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::PerformanceSettings)
    SE_FIELD(target_fps)
    SE_FIELD(busy_wait_ratio)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::EditorConfig)
    SE_FIELD(window)
    SE_FIELD(graphics)
    SE_FIELD(performance)
    SE_FIELD(schemes)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::IntFields)
    SE_FIELD(i8_value)
    SE_FIELD(u8_value)
    SE_FIELD(i16_value)
    SE_FIELD(u16_value)
    SE_FIELD(i32_value)
    SE_FIELD(u32_value)
    SE_FIELD(i64_value)
    SE_FIELD(u64_value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::FloatFields)
    SE_FIELD(single)
    SE_FIELD(wide)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::Item)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasItems)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasMap)
    SE_FIELD(scores)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasItemMap)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasIdMap)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasModeMap)
    SE_FIELD(modes)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasU64Map)
    SE_FIELD(values)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasOptional)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasDefaultSome)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasOptionalArray)
    SE_FIELD(values)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasNestedOptional)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasSets)
    SE_FIELD(ints)
    SE_FIELD(floats)
    SE_FIELD(flags)
    SE_FIELD(names)
    SE_FIELD(modes)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::Vec3)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::Transform)
    SE_FIELD(name)
    SE_FIELD(position)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasTransforms)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasNumbers)
    SE_FIELD(numbers)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasEmpties)
    SE_FIELD(numbers)
    SE_FIELD(scores)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_json_test::HasText)
    SE_FIELD(text)
SE_REFLECT_END()


namespace
{
/** value를 JSON 텍스트로 씁니다. 쓰기에 실패하면 테스트를 실패시키고 빈 문자열을 돌려줍니다. */
template <typename T>
String WriteToText(const T& value)
{
    JsonWriter writer;
    const auto written = serde::Serialize(writer, value);
    EXPECT_TRUE(written.HasValue()) << (written.HasError() ? written.Error().message.CStr() : "");

    const auto text = writer.ToText();
    EXPECT_TRUE(text.HasValue()) << (text.HasError() ? text.Error().CStr() : "");
    return text.HasValue() ? text.Value() : String{};
}

/** JSON 텍스트를 T로 읽습니다. 읽기에 실패하면 테스트를 실패시키고 기본값을 돌려줍니다. */
template <typename T>
T ReadFromText(StringView text)
{
    JsonReader reader(text);
    T result{};
    const auto read_result = serde::Deserialize(reader, result);
    EXPECT_TRUE(read_result.HasValue()) << std::string_view{ text } << "\n" << (read_result.HasError() ? read_result.Error().message.CStr() : "");
    return result;
}

/** JSON 텍스트를 T로 읽다가 난 오류를 돌려줍니다. 오류가 없으면 테스트를 실패시킵니다. */
template <typename T>
SerializeError ReadError(StringView text)
{
    JsonReader reader(text);
    T result{};
    const auto read_result = serde::Deserialize(reader, result);
    EXPECT_TRUE(read_result.HasError()) << std::string_view{ text };
    return read_result.HasError() ? read_result.Error() : SerializeError{ .message = "no error" };
}

/** value를 쓰다가 난 오류를 돌려줍니다. 오류가 없으면 "no error" 메시지를 돌려줍니다. */
template <typename T>
SerializeError WriteError(const T& value)
{
    JsonWriter writer;
    const auto write_result = serde::Serialize(writer, value);
    return write_result.HasError() ? write_result.Error() : SerializeError{ .message = "no error" };
}
} // namespace


// --- 왕복 ---

TEST(JsonWriterReaderTest, SettingsRoundTrip)
{
    using namespace se_json_test;

    const EditorConfig original{
        .window = WindowSettings{ .title = "Editor", .width = 1600, .height = 900, .fullscreen = true },
        .graphics = GraphicsSettings{ .present_mode = EPresentMode::VSync },
        .performance = PerformanceSettings{ .target_fps = 144, .busy_wait_ratio = 0.25f },
        .schemes = { "CoreAssets", "EditorAssets", "GameAssets" },
    };
    const String text = WriteToText(original);

    EXPECT_EQ(ReadFromText<EditorConfig>(text), original);
}

TEST(JsonWriterReaderTest, ArrayOfStructsRoundTrip)
{
    using namespace se_json_test;

    HasItems original;
    original.items.Push(Item{ .value = 1 });
    original.items.Push(Item{ .value = 2 });
    const String text = WriteToText(original);

    EXPECT_EQ(ReadFromText<HasItems>(text).items, original.items);
}

TEST(JsonWriterReaderTest, RootCanBeAnyValue)
{
    // JSON은 struct가 아닌 값도 문서의 루트가 될 수 있음
    EXPECT_EQ(WriteToText(i32{ 42 }), "42\n");
    EXPECT_EQ(ReadFromText<i32>("42"), 42);

    const Array<i32> numbers = { 1, 2 };
    EXPECT_EQ(WriteToText(numbers), "[\n    1,\n    2\n]\n");
    EXPECT_EQ(ReadFromText<Array<i32>>("[ 1, 2 ]"), numbers);
}


// --- 정수 ---

TEST(JsonWriterReaderTest, IntBoundariesRoundTrip)
{
    using namespace se_json_test;

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
        const String text = WriteToText(original);
        EXPECT_EQ(ReadFromText<IntFields>(text), original) << text.CStr();
    }
}

TEST(JsonWriterReaderTest, IntegersAboveTwoToThe53AreWrittenAsStrings)
{
    using namespace se_json_test;

    // 2^53 - 1까지는 JSON 숫자, 그 위는 10진 문자열
    const String safe = WriteToText(IntFields{ .i64_value = 9007199254740991, .u64_value = 9007199254740991 });
    EXPECT_TRUE(safe.Contains("\"i64_value\": 9007199254740991")) << safe.CStr();
    EXPECT_TRUE(safe.Contains("\"u64_value\": 9007199254740991")) << safe.CStr();

    const String negative_safe = WriteToText(IntFields{ .i64_value = -9007199254740991 });
    EXPECT_TRUE(negative_safe.Contains("\"i64_value\": -9007199254740991")) << negative_safe.CStr();

    const IntFields unsafe_values{
        .i64_value = std::numeric_limits<i64>::min(),
        .u64_value = std::numeric_limits<u64>::max(),
    };
    const String unsafe = WriteToText(unsafe_values);
    EXPECT_TRUE(unsafe.Contains("\"i64_value\": \"-9223372036854775808\"")) << unsafe.CStr();
    EXPECT_TRUE(unsafe.Contains("\"u64_value\": \"18446744073709551615\"")) << unsafe.CStr();
    EXPECT_EQ(ReadFromText<IntFields>(unsafe), unsafe_values);

    const IntFields just_above{ .i64_value = 9007199254740992, .u64_value = 9007199254740992 };
    const String above = WriteToText(just_above);
    EXPECT_TRUE(above.Contains("\"i64_value\": \"9007199254740992\"")) << above.CStr();
    EXPECT_TRUE(above.Contains("\"u64_value\": \"9007199254740992\"")) << above.CStr();
    EXPECT_EQ(ReadFromText<IntFields>(above), just_above);

    const IntFields just_below{ .i64_value = -9007199254740992 };
    const String below = WriteToText(just_below);
    EXPECT_TRUE(below.Contains("\"i64_value\": \"-9007199254740992\"")) << below.CStr();
    EXPECT_EQ(ReadFromText<IntFields>(below), just_below);
}

TEST(JsonWriterReaderTest, IntegersAreReadFromNumbersOrDecimalStrings)
{
    using namespace se_json_test;

    // 사람이 적은 2^53을 넘는 숫자도 정확한 정수로 읽힘
    const IntFields big = ReadFromText<IntFields>(R"({ "u64_value": 9007199254740993, "i64_value": 9007199254740993 })");
    EXPECT_EQ(big.u64_value, 9007199254740993u);
    EXPECT_EQ(big.i64_value, 9007199254740993);

    const IntFields max_u64 = ReadFromText<IntFields>(R"({ "u64_value": 18446744073709551615 })");
    EXPECT_EQ(max_u64.u64_value, std::numeric_limits<u64>::max());

    // 작은 정수도 10진 문자열로 받음
    const IntFields strings = ReadFromText<IntFields>(R"({ "i32_value": "-5", "u8_value": "200", "u64_value": "7" })");
    EXPECT_EQ(strings.i32_value, -5);
    EXPECT_EQ(strings.u8_value, 200u);
    EXPECT_EQ(strings.u64_value, 7u);
}

TEST(JsonWriterReaderTest, IntReadErrors)
{
    struct Case
    {
        std::string_view text;
        const char* path;
        const char* message;
    };
    const Case cases[] = {
        { R"({ "u8_value": 256 })", "u8_value", "JsonReader: 256 is out of range for u8." },
        { R"({ "i8_value": -129 })", "i8_value", "JsonReader: -129 is out of range for i8." },
        { R"({ "u32_value": -1 })", "u32_value", "JsonReader: -1 is out of range for u32." },
        { R"({ "u64_value": -1 })", "u64_value", "JsonReader: -1 is out of range for u64." },
        { R"({ "u8_value": 18446744073709551615 })", "u8_value", "JsonReader: 18446744073709551615 is out of range for u8." },
        { R"({ "i32_value": 25.0 })", "i32_value", "JsonReader: expected an integer, got a float." },
        { R"({ "i64_value": true })", "i64_value", "JsonReader: expected an integer, got a boolean." },
        { R"({ "u64_value": "abc" })", "u64_value", "JsonReader: 'abc' is not a valid u64 number." },
        { R"({ "i32_value": "12x" })", "i32_value", "JsonReader: '12x' is not a valid i32 number." },
        { R"({ "i32_value": "5000000000" })", "i32_value", "JsonReader: 5000000000 is out of range for i32." },
        { R"({ "i64_value": "99999999999999999999" })", "i64_value", "JsonReader: 99999999999999999999 is out of range for i64." },
    };

    for (const Case& c : cases)
    {
        const SerializeError error = ReadError<se_json_test::IntFields>(c.text);
        EXPECT_EQ(error.path, c.path) << c.text;
        EXPECT_EQ(error.message, c.message) << c.text;
    }
}


// --- 실수 ---

TEST(JsonWriterReaderTest, F32IsWrittenAsShortestText)
{
    // 레거시는 f32를 넓힌 값 그대로 써서 0.10000000149011612가 됨
    const String text = WriteToText(se_json_test::PerformanceSettings{});
    EXPECT_EQ(text, "{ \"target_fps\": 240, \"busy_wait_ratio\": 0.1 }\n");
}

TEST(JsonWriterReaderTest, FloatsAlwaysHaveDecimalPointOrExponent)
{
    using namespace se_json_test;

    // 정수처럼 보이는 실수는 ".0"을 붙여 정수와 구분하고, 지수 표기는 그대로 둠
    EXPECT_EQ(WriteToText(FloatFields{ .single = 17.0f, .wide = 2.0 }), "{ \"single\": 17.0, \"wide\": 2.0 }\n");
    EXPECT_EQ(WriteToText(FloatFields{ .single = -0.0f, .wide = 0.5 }), "{ \"single\": -0.0, \"wide\": 0.5 }\n");
    EXPECT_EQ(WriteToText(FloatFields{ .single = 0.0f, .wide = 1e20 }), "{ \"single\": 0.0, \"wide\": 1e+20 }\n");
}

TEST(JsonWriterReaderTest, F32RoundTripsExactly)
{
    using namespace se_json_test;

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
        // 가장 짧은 표현(7.038531e-26)을 f64로 읽으면 두 f32의 정확한 중간값이 되는 값
        std::bit_cast<f32>(0x15ae43fdu),
        std::bit_cast<f32>(0x95ae43fdu),
    };

    for (const f32 value : values)
    {
        const String text = WriteToText(FloatFields{ .single = value });
        const FloatFields result = ReadFromText<FloatFields>(text);
        EXPECT_EQ(std::bit_cast<u32>(result.single), std::bit_cast<u32>(value)) << value << " " << text.CStr();
    }
}

TEST(JsonWriterReaderTest, NanAndInfinityAreWriteErrors)
{
    constexpr const char* MESSAGE = "JsonWriter: NaN and infinity cannot be written in JSON.";

    {
        JsonWriter writer;
        writer.Float(std::numeric_limits<f64>::quiet_NaN(), EFloatWidth::Bits64);
        ASSERT_TRUE(writer.HasError());
        EXPECT_EQ(String(writer.GetError()), MESSAGE);
    }
    {
        JsonWriter writer;
        writer.Float(-std::numeric_limits<f64>::infinity(), EFloatWidth::Bits64);
        ASSERT_TRUE(writer.HasError());
        EXPECT_EQ(String(writer.GetError()), MESSAGE);
    }
    {
        JsonWriter writer;
        writer.Float(std::numeric_limits<f32>::infinity(), EFloatWidth::Bits32);
        ASSERT_TRUE(writer.HasError());
        EXPECT_EQ(String(writer.GetError()), MESSAGE);
    }
    {
        const SerializeError error = WriteError(se_json_test::FloatFields{ .single = std::numeric_limits<f32>::quiet_NaN() });
        EXPECT_EQ(error.path, "single");
        EXPECT_EQ(error.message, MESSAGE);
    }
}

TEST(JsonWriterReaderTest, FloatReadRules)
{
    using namespace se_json_test;

    {
        // 사람이 정수로 적은 실수도 받음
        const FloatFields result = ReadFromText<FloatFields>(R"({ "single": 17, "wide": 2 })");
        EXPECT_EQ(result.single, 17.0f);
        EXPECT_EQ(result.wide, 2.0);
    }
    {
        // FLT_MAX의 가장 짧은 표현은 FLT_MAX보다 조금 크지만 반올림하면 FLT_MAX
        const FloatFields result = ReadFromText<FloatFields>(R"({ "single": 3.4028235e+38 })");
        EXPECT_EQ(result.single, std::numeric_limits<f32>::max());
    }
    {
        const SerializeError error = ReadError<FloatFields>(R"({ "single": 1e39 })");
        EXPECT_EQ(error.path, "single");
        EXPECT_EQ(error.message, "JsonReader: 1e+39 is out of range for f32.");
    }
    {
        const SerializeError error = ReadError<FloatFields>(R"({ "wide": true })");
        EXPECT_EQ(error.message, "JsonReader: expected a float, got a boolean.");
    }
}


// --- enum ---

TEST(JsonWriterReaderTest, EnumIsWrittenByName)
{
    using namespace se_json_test;

    EXPECT_EQ(WriteToText(GraphicsSettings{ .present_mode = EPresentMode::Immediate }), "{ \"present_mode\": \"Immediate\" }\n");

    // 이름이 없는 값은 정수로 씀
    const String unnamed = WriteToText(GraphicsSettings{ .present_mode = static_cast<EPresentMode>(7) });
    EXPECT_EQ(unnamed, "{ \"present_mode\": 7 }\n");
    EXPECT_EQ(ReadFromText<GraphicsSettings>(unnamed).present_mode, static_cast<EPresentMode>(7));
}

TEST(JsonWriterReaderTest, EnumIsReadByNameOrInteger)
{
    using namespace se_json_test;

    EXPECT_EQ(ReadFromText<GraphicsSettings>(R"({ "present_mode": "VSync" })").present_mode, EPresentMode::VSync);
    EXPECT_EQ(ReadFromText<GraphicsSettings>(R"({ "present_mode": 2 })").present_mode, EPresentMode::Immediate);

    {
        const SerializeError error = ReadError<GraphicsSettings>(R"({ "present_mode": "Fast" })");
        EXPECT_EQ(error.path, "present_mode");
        EXPECT_EQ(error.message, "JsonReader: 'Fast' is not a name of this enum.");
    }
    {
        const SerializeError error = ReadError<GraphicsSettings>(R"({ "present_mode": 300 })");
        EXPECT_EQ(error.message, "JsonReader: 300 is out of range for u8.");
    }
}

TEST(JsonWriterReaderTest, UnnamedEnumAboveTwoToThe53IsError)
{
    // 문자열로 쓰면 이름으로 읽혀 되읽을 수 없으므로 쓰기에서 막음
    JsonWriter writer;
    writer.BeginStruct();
    writer.Field("flag");
    writer.Enum(-1, EIntWidth::Bits64, false, {});

    ASSERT_TRUE(writer.HasError());
    EXPECT_EQ(String(writer.GetError()), "JsonWriter: enum value 18446744073709551615 has no name and does not fit in a JSON number.");
}


// --- 맵 ---

TEST(JsonWriterReaderTest, StringKeyMapIsWrittenAsObjectInKeyOrder)
{
    using namespace se_json_test;

    const HasMap original{ .scores = { { "bob", 87 }, { "alice", 95 } } };
    const String text = WriteToText(original);

    EXPECT_EQ(text, "{\n    \"scores\": { \"alice\": 95, \"bob\": 87 }\n}\n");
    EXPECT_EQ(ReadFromText<HasMap>(text), original);
}

TEST(JsonWriterReaderTest, EnumKeyMapIsWrittenAsObjectOfNames)
{
    using namespace se_json_test;

    const HasModeMap original{ .modes = { { EPresentMode::VSync, 60 }, { EPresentMode::Immediate, 0 } } };
    const String text = WriteToText(original);

    // 객체의 키를 enum 이름으로 읽음
    EXPECT_EQ(text, "{\n    \"modes\": { \"Immediate\": 0, \"VSync\": 60 }\n}\n");
    EXPECT_EQ(ReadFromText<HasModeMap>(text), original);
}

TEST(JsonWriterReaderTest, NonStringKeyMapIsWrittenAsSortedPairs)
{
    using namespace se_json_test;

    const HasIdMap original{
        .items = { { 10, Item{ .value = 100 } }, { -2, Item{ .value = -20 } }, { 3, Item{ .value = 30 } } },
    };
    const String text = WriteToText(original);

    // key 순서로 정렬한 [key, value] 쌍 배열
    constexpr char EXPECTED[] = R"({
    "items": [
        [
            -2,
            { "value": -20 }
        ],
        [
            3,
            { "value": 30 }
        ],
        [
            10,
            { "value": 100 }
        ]
    ]
}
)";
    EXPECT_EQ(text, EXPECTED);
    EXPECT_EQ(ReadFromText<HasIdMap>(text), original);
}

TEST(JsonWriterReaderTest, LargeU64KeysAreObjectKeys)
{
    using namespace se_json_test;

    // 2^53 - 1을 넘는 u64는 10진 문자열로 쓰이므로 key가 모두 그렇다면 객체가 되고, 객체의 키를 정수로 읽음
    const HasU64Map original{ .values = { { std::numeric_limits<u64>::max(), 1 } } };
    const String text = WriteToText(original);

    EXPECT_EQ(text, "{\n    \"values\": { \"18446744073709551615\": 1 }\n}\n");
    EXPECT_EQ(ReadFromText<HasU64Map>(text), original);
}

TEST(JsonWriterReaderTest, EmptyMapIsWrittenAsEmptyObject)
{
    using namespace se_json_test;

    EXPECT_EQ(WriteToText(HasIdMap{}), "{\n    \"items\": {}\n}\n");

    // 빈 맵은 빈 객체와 빈 배열을 모두 받고, 기존 엔트리는 지움
    for (const std::string_view text : { R"({ "items": {} })", R"({ "items": [] })" })
    {
        JsonReader reader{ StringView{ text } };
        HasIdMap result{ .items = { { 1, Item{ .value = 1 } } } };
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue()) << text;
        EXPECT_TRUE(result.items.IsEmpty()) << text;
    }
}

TEST(JsonWriterReaderTest, MapIsReadFromObjectOrPairs)
{
    using namespace se_json_test;

    const HasMap expected{ .scores = { { "alice", 1 }, { "bob", 2 } } };
    for (const std::string_view text : {
        R"({ "scores": { "alice": 1, "bob": 2 } })",
        R"({ "scores": [ [ "bob", 2 ], [ "alice", 1 ] ] })",
    })
    {
        EXPECT_EQ(ReadFromText<HasMap>(StringView{ text }), expected) << text;
    }
}

TEST(JsonWriterReaderTest, MapReadErrors)
{
    struct Case
    {
        std::string_view text;
        const char* path;
        const char* message;
    };
    const Case cases[] = {
        { R"({ "scores": 5 })", "scores", "JsonReader: expected an object or an array, got an integer." },
        { R"({ "scores": { "alice": true } })", "scores[0].value", "JsonReader: expected an integer, got a boolean." },
        { R"({ "scores": [ "alice" ] })", "scores[0].key", "JsonReader: expected a [key, value] array, got a string." },
        { R"({ "scores": [ [ "alice" ] ] })", "scores[0].key", "JsonReader: expected a [key, value] array, got an array of length 1." },
        { R"({ "scores": [ [ 1, 2 ] ] })", "scores[0].key", "JsonReader: expected a string, got an integer." },
    };

    for (const Case& c : cases)
    {
        const SerializeError error = ReadError<se_json_test::HasMap>(c.text);
        EXPECT_EQ(error.path, c.path) << c.text;
        EXPECT_EQ(error.message, c.message) << c.text;
    }
}

TEST(JsonWriterReaderTest, DuplicateObjectKeyIsError)
{
    // 서로 다른 key가 같은 문자열로 쓰이면 객체에서 한쪽이 사라지므로 오류
    JsonWriter writer;
    writer.BeginMap(2);
    for (const i64 value : { 1, 2 })
    {
        writer.BeginMapEntry();
        writer.Str("same");
        writer.Int(value, EIntWidth::Bits32, true);
        writer.EndMapEntry();
    }
    writer.EndMap();

    ASSERT_TRUE(writer.HasError());
    EXPECT_EQ(String(writer.GetError()), "JsonWriter: map key 'same' is written twice.");
}


// --- 정렬 ---

TEST(JsonWriterReaderTest, UnorderedSequencesAreWrittenSorted)
{
    using namespace se_json_test;

    const HasSets original{
        .ints = { 30, -5, 7, 0 },
        .floats = { 2.5, -1.0, 0.5 },
        .flags = { true, false },
        .names = { "b", "a", "B" },
        .modes = { EPresentMode::VSync, static_cast<EPresentMode>(7), EPresentMode::Mailbox },
    };
    const String text = WriteToText(original);

    // 정수와 실수는 값, 문자열은 바이트 사전순, bool은 false가 먼저. 이름 없는 enum 값(7)은 숫자라 종류 순서로 문자열 앞
    constexpr char EXPECTED[] = R"({
    "ints": [
        -5,
        0,
        7,
        30
    ],
    "floats": [
        -1.0,
        0.5,
        2.5
    ],
    "flags": [
        false,
        true
    ],
    "names": [
        "B",
        "a",
        "b"
    ],
    "modes": [
        7,
        "Mailbox",
        "VSync"
    ]
}
)";
    EXPECT_EQ(text, EXPECTED);
    EXPECT_EQ(ReadFromText<HasSets>(text), original);
}

TEST(JsonWriterReaderTest, SameValueGivesSameText)
{
    using namespace se_json_test;

    // 같은 내용을 반대 순서로 넣어 HashMap과 HashSet의 순회 순서가 달라도 텍스트는 같음
    HasIdMap forward_map;
    HasIdMap backward_map;
    HasSets forward_sets;
    HasSets backward_sets;
    for (const i32 value : std::views::iota(0, 100))
    {
        forward_map.items.Insert(value, Item{ .value = value });
        forward_sets.ints.Insert(value);
        forward_sets.names.Insert(String::Format("name{}", value));
    }
    for (const i32 value : std::views::iota(0, 100) | std::views::reverse)
    {
        backward_map.items.Insert(value, Item{ .value = value });
        backward_sets.ints.Insert(value);
        backward_sets.names.Insert(String::Format("name{}", value));
    }

    EXPECT_EQ(WriteToText(forward_map), WriteToText(backward_map));
    EXPECT_EQ(WriteToText(forward_sets), WriteToText(backward_sets));
}


// --- Optional ---

TEST(JsonWriterReaderTest, NoneFieldIsOmitted)
{
    using namespace se_json_test;

    // None은 키를 쓰지 않고, Some은 값을 그대로 씀
    EXPECT_EQ(WriteToText(HasOptional{}), "{}\n");
    EXPECT_EQ(WriteToText(HasOptional{ .value = 3 }), "{ \"value\": 3 }\n");

    for (const HasOptional& original : { HasOptional{}, HasOptional{ .value = 3 } })
    {
        EXPECT_EQ(ReadFromText<HasOptional>(WriteToText(original)), original);
    }
}

TEST(JsonWriterReaderTest, MissingOptionalFieldIsReadAsNone)
{
    using namespace se_json_test;

    // 기본값이 Some이어도 키가 없으면 None
    JsonReader reader("{}");
    HasDefaultSome result;
    ASSERT_TRUE(result.value.HasValue());
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_FALSE(result.value.HasValue());
}

TEST(JsonWriterReaderTest, NoneOutsideStructFieldIsError)
{
    using namespace se_json_test;

    // 시퀀스 원소의 None은 생략하면 그 자리가 사라짐
    const SerializeError in_array = WriteError(HasOptionalArray{ .values = { 1, NullOpt } });
    EXPECT_EQ(in_array.path, "values[1]");
    EXPECT_EQ(in_array.message, "JsonWriter: None can only be written as a struct field, by omitting its key.");

    // Some(None)은 키를 생략하면 바깥 None으로 읽힘
    HasNestedOptional nested_value;
    nested_value.value.Emplace();
    const SerializeError nested = WriteError(nested_value);
    EXPECT_EQ(nested.path, "value");
    EXPECT_EQ(nested.message, "JsonWriter: None inside another Optional cannot be written because the omitted key reads back as the outer None.");
}


// --- Bytes ---

TEST(JsonWriterReaderTest, BytesAreWrittenAsBase64)
{
    const u8 original[] = { 'M', 'a', 'n', 0xFF };

    JsonWriter writer;
    writer.BeginStruct();
    writer.Field("blob");
    writer.Bytes(original, sizeof(original));
    writer.EndStruct();
    ASSERT_FALSE(writer.HasError());

    const auto text = writer.ToText();
    ASSERT_TRUE(text.HasValue());
    EXPECT_EQ(text.Value(), "{ \"blob\": \"TWFu/w==\" }\n");

    JsonReader reader(text.Value());
    reader.BeginStruct();
    ASSERT_TRUE(reader.Field("blob"));
    u8 result[4] = {};
    reader.Bytes(result, sizeof(result));
    reader.EndStruct();
    ASSERT_FALSE(reader.HasError());
    EXPECT_TRUE(std::ranges::equal(result, original));
}

TEST(JsonWriterReaderTest, BytesReadErrors)
{
    struct Case
    {
        std::string_view text;
        const char* message;
    };
    const Case cases[] = {
        { R"({ "blob": "TWF" })", "JsonReader: invalid base64 string." },
        { R"({ "blob": "TWFu/w==" })", "JsonReader: expected 3 bytes, got 4 bytes of base64 data." },
        { R"({ "blob": 3 })", "JsonReader: expected a base64 string, got an integer." },
    };

    for (const Case& c : cases)
    {
        JsonReader reader{ StringView{ c.text } };
        reader.BeginStruct();
        ASSERT_TRUE(reader.Field("blob")) << c.text;
        u8 result[3] = {};
        reader.Bytes(result, sizeof(result));
        EXPECT_EQ(String(reader.GetError()), c.message) << c.text;
    }
}


// --- 경고 ---

TEST(JsonWriterReaderTest, UnknownKeysAreWarnings)
{
    using namespace se_json_test;

    JsonReader reader(R"({ "typo_at_root": 1, "window": { "widht": 900, "width": 1600 } })");
    EditorConfig result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

    // 경고가 있어도 아는 키는 읽고, 오타 난 키의 필드와 아예 없는 필드는 기본값으로 남음
    EXPECT_EQ(result.window.width, 1600u);
    EXPECT_EQ(result.window.height, 720u);
    EXPECT_EQ(result.window.title, "SimpleEngine");
    EXPECT_EQ(result.schemes, (Array<String>{ "CoreAssets", "EditorAssets" }));

    // window를 먼저 닫으므로 window의 키가 먼저
    const ArrayView<const String> warnings = reader.GetWarnings();
    ASSERT_EQ(warnings.Len(), 2u);
    EXPECT_EQ(warnings[0], "JsonReader: unknown key 'window.widht' is ignored.");
    EXPECT_EQ(warnings[1], "JsonReader: unknown key 'typo_at_root' is ignored.");
}

TEST(JsonWriterReaderTest, UnknownKeyInArrayElementHasIndexedPath)
{
    JsonReader reader(R"({ "items": [ { "value": 1 }, { "value": 2, "extra": true } ] })");
    se_json_test::HasItems result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

    const ArrayView<const String> warnings = reader.GetWarnings();
    ASSERT_EQ(warnings.Len(), 1u);
    EXPECT_EQ(warnings[0], "JsonReader: unknown key 'items[1].extra' is ignored.");
}

TEST(JsonWriterReaderTest, UnknownKeyInMapValueHasEntryPath)
{
    {
        // 객체 맵의 value는 "맵.키"
        JsonReader reader(R"({ "items": { "alice": { "value": 1, "extra": true } } })");
        se_json_test::HasItemMap result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

        const ArrayView<const String> warnings = reader.GetWarnings();
        ASSERT_EQ(warnings.Len(), 1u);
        EXPECT_EQ(warnings[0], "JsonReader: unknown key 'items.alice.extra' is ignored.");
    }
    {
        // 쌍 배열 맵의 value는 "맵[엔트리 번호][1]"
        JsonReader reader(R"({ "items": [ [ 7, { "value": 1, "extra": true } ] ] })");
        se_json_test::HasIdMap result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

        const ArrayView<const String> warnings = reader.GetWarnings();
        ASSERT_EQ(warnings.Len(), 1u);
        EXPECT_EQ(warnings[0], "JsonReader: unknown key 'items[0][1].extra' is ignored.");
    }
}

TEST(JsonWriterReaderTest, NoWarningsForWrittenText)
{
    // JsonWriter가 쓴 텍스트에는 타입에 없는 키가 없음
    const String text = WriteToText(se_json_test::EditorConfig{});

    JsonReader reader(text);
    se_json_test::EditorConfig result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
}


// --- 파싱 오류 ---

TEST(JsonWriterReaderTest, ParseErrorHasLineAndColumn)
{
    // 두 번째 줄의 끝 쉼표
    JsonReader reader("{\n\"a\": [1, 2,]\n}");
    ASSERT_TRUE(reader.HasError());
    EXPECT_TRUE(String(reader.GetError()).StartsWith("JsonReader: failed to parse JSON at line 2, column ")) << std::string_view{ reader.GetError() };

    // 파싱에 실패한 리더로 읽으면 같은 오류가 남음
    se_json_test::HasNumbers result;
    const auto read_result = serde::Deserialize(reader, result);
    ASSERT_TRUE(read_result.HasError());
    EXPECT_TRUE(read_result.Error().message.StartsWith("JsonReader: failed to parse JSON at line 2, column "));
}

TEST(JsonWriterReaderTest, NonStandardJsonIsParseError)
{
    for (const std::string_view text : {
        R"({ /* comment */ "a": 1 })",
        "{ // comment\n\"a\": 1 }",
        R"({ "a": [1, 2,] })",
        R"({ "a": 1, })",
        R"({ "a": NaN })",
        R"({ "a": Infinity })",
        R"({ 'a': 1 })",
        R"({ "a": 1 } extra)",
        "",
    })
    {
        JsonReader reader{ StringView{ text } };
        ASSERT_TRUE(reader.HasError()) << text;
        EXPECT_TRUE(String(reader.GetError()).StartsWith("JsonReader: failed to parse JSON at line ")) << text;
    }
}

TEST(JsonWriterReaderTest, DuplicateKeyIsErrorWithLocation)
{
    struct Case
    {
        std::string_view text;
        const char* message;
    };
    const Case cases[] = {
        { R"({ "a": 1, "a": 2 })", "JsonReader: duplicate key 'a' at '(root)'." },
        { R"({ "window": { "width": 1, "width": 2 } })", "JsonReader: duplicate key 'width' at 'window'." },
        { R"({ "items": [ { "value": 1 }, { "value": 1, "value": 2 } ] })", "JsonReader: duplicate key 'value' at 'items[1]'." },
    };

    for (const Case& c : cases)
    {
        JsonReader reader{ StringView{ c.text } };
        ASSERT_TRUE(reader.HasError()) << c.text;
        EXPECT_EQ(String(reader.GetError()), c.message) << c.text;
    }
}

TEST(JsonWriterReaderTest, DeeplyNestedDocumentIsError)
{
    String text;
    for (usize depth = 0; depth < 300; ++depth)
    {
        text.Append("[");
    }
    for (usize depth = 0; depth < 300; ++depth)
    {
        text.Append("]");
    }

    JsonReader reader(text);
    ASSERT_TRUE(reader.HasError());
    EXPECT_EQ(String(reader.GetError()), "JsonReader: the document is nested deeper than 256 levels.");
}


// --- 구간과 Rewind ---

TEST(JsonWriterReaderTest, SectionsWriteNothing)
{
    const auto write = [](bool with_section)
    {
        JsonWriter writer;
        writer.BeginStruct();
        writer.Field("id");
        writer.Int(1, EIntWidth::Bits32, true);
        writer.Field("components");
        if (with_section)
        {
            writer.BeginSection();
        }
        writer.BeginStruct();
        writer.Field("x");
        writer.Int(2, EIntWidth::Bits32, true);
        writer.EndStruct();
        if (with_section)
        {
            writer.EndSection();
        }
        writer.EndStruct();
        EXPECT_FALSE(writer.HasError());
        return writer.ToText().Value();
    };

    EXPECT_EQ(write(true), write(false));
}

TEST(JsonWriterReaderTest, SkipSectionTakesOneValue)
{
    {
        JsonReader reader(R"({ "id": 7, "items": [ 1, 2 ], "components": { "x": 1 } })");
        i64 second = 0;
        i64 id = 0;
        reader.BeginStruct();
        ASSERT_TRUE(reader.Field("components"));
        reader.SkipSection();
        ASSERT_TRUE(reader.Field("items"));
        u64 count = 0;
        reader.BeginSeq(count);
        reader.SkipSection(); // 원소 하나
        reader.Int(second, EIntWidth::Bits32, true);
        reader.EndSeq();
        ASSERT_TRUE(reader.Field("id"));
        reader.Int(id, EIntWidth::Bits32, true);
        reader.EndStruct();

        EXPECT_FALSE(reader.HasError());
        EXPECT_EQ(second, 2);
        EXPECT_EQ(id, 7);
        // 건너뛴 키도 타입이 물어본 키라 경고가 없음
        EXPECT_TRUE(reader.GetWarnings().IsEmpty());
    }
    {
        JsonReader reader(R"({ "x": 1 })");
        reader.BeginStruct();
        reader.SkipSection();

        ASSERT_TRUE(reader.HasError());
        EXPECT_EQ(String(reader.GetError()), "JsonReader: a value inside a struct needs a Field name first.");
    }
}

TEST(JsonWriterReaderTest, RewindReadsSameValuesAgain)
{
    using namespace se_json_test;

    const EditorConfig original{
        .window = WindowSettings{ .title = "Editor", .width = 1600 },
    };
    const String text = WriteToText(original);

    JsonReader reader(text);
    EditorConfig first;
    ASSERT_TRUE(serde::Deserialize(reader, first).HasValue());

    reader.Rewind();
    EditorConfig second;
    ASSERT_TRUE(serde::Deserialize(reader, second).HasValue());

    // 읽는 도중에 되감아도 루트부터 다시 읽힘
    reader.Rewind();
    reader.BeginStruct();
    ASSERT_TRUE(reader.Field("window"));
    reader.BeginStruct();
    reader.Rewind();
    EditorConfig third;
    ASSERT_TRUE(serde::Deserialize(reader, third).HasValue());

    EXPECT_EQ(first, original);
    EXPECT_EQ(second, original);
    EXPECT_EQ(third, original);
}

TEST(JsonWriterReaderTest, RewindClearsWarnings)
{
    JsonReader reader(R"({ "typo": 1 })");
    se_json_test::EditorConfig result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    ASSERT_EQ(reader.GetWarnings().Len(), 1u);

    reader.Rewind();
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());

    // 두 번 읽어도 경고는 한 번 분량
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(reader.GetWarnings().Len(), 1u);
}


// --- 출력 모양 ---

TEST(JsonWriterReaderTest, NestedStructGolden)
{
    using namespace se_json_test;

    // 스칼라만 담은 객체(position)는 한 줄, 객체를 담은 객체는 멤버마다 한 줄
    const Transform original{ .name = "a", .position = Vec3{ .x = 1.0f, .y = 2.0f, .z = 3.0f } };
    constexpr char EXPECTED[] = R"({
    "name": "a",
    "position": { "x": 1.0, "y": 2.0, "z": 3.0 }
}
)";
    EXPECT_EQ(WriteToText(original), EXPECTED);
    EXPECT_EQ(ReadFromText<Transform>(EXPECTED), original);
}

TEST(JsonWriterReaderTest, StructArrayGolden)
{
    using namespace se_json_test;

    HasTransforms original;
    original.items.Push(Transform{ .name = "a", .position = Vec3{ .x = 1.0f, .y = 2.0f, .z = 3.0f } });
    original.items.Push(Transform{ .name = "b", .position = Vec3{ .x = 0.5f, .y = 0.0f, .z = -1.0f } });

    constexpr char EXPECTED[] = R"({
    "items": [
        {
            "name": "a",
            "position": { "x": 1.0, "y": 2.0, "z": 3.0 }
        },
        {
            "name": "b",
            "position": { "x": 0.5, "y": 0.0, "z": -1.0 }
        }
    ]
}
)";
    EXPECT_EQ(WriteToText(original), EXPECTED);
    EXPECT_EQ(ReadFromText<HasTransforms>(EXPECTED), original);
}

TEST(JsonWriterReaderTest, ScalarArrayHasOneElementPerLine)
{
    using namespace se_json_test;

    const HasNumbers original{ .numbers = { 1, 2, 3 } };
    constexpr char EXPECTED[] = R"({
    "numbers": [
        1,
        2,
        3
    ]
}
)";
    EXPECT_EQ(WriteToText(original), EXPECTED);
    EXPECT_EQ(ReadFromText<HasNumbers>(EXPECTED), original);
}

TEST(JsonWriterReaderTest, EmptyArrayAndEmptyObjectGolden)
{
    using namespace se_json_test;

    constexpr char EXPECTED[] = R"({
    "numbers": [],
    "scores": {}
}
)";
    EXPECT_EQ(WriteToText(HasEmpties{}), EXPECTED);

    const HasEmpties result = ReadFromText<HasEmpties>(EXPECTED);
    EXPECT_TRUE(result.numbers.IsEmpty());
    EXPECT_TRUE(result.scores.IsEmpty());
}

TEST(JsonWriterReaderTest, KoreanTextAndEscapesGolden)
{
    using namespace se_json_test;

    // 비ASCII는 UTF-8 그대로, 따옴표, 역슬래시, 제어 문자는 이스케이프
    const HasText original{ .text = "한글 \"따옴표\" \\ \n\t\x01" };
    constexpr char EXPECTED[] = R"({ "text": "한글 \"따옴표\" \\ \n\t\u0001" }
)";
    EXPECT_EQ(WriteToText(original), EXPECTED);
    EXPECT_EQ(ReadFromText<HasText>(EXPECTED), original);

    // 사람이 \uXXXX로 적은 한글도 읽힘
    EXPECT_EQ(ReadFromText<HasText>(R"({ "text": "한글" })").text, "한글");
}


// --- 오류 ---

TEST(JsonWriterReaderTest, NodeOrderMistakesAreErrors)
{
    {
        // Field 없이 값을 씀
        JsonWriter writer;
        writer.BeginStruct();
        writer.Int(1, EIntWidth::Bits32, true);
        EXPECT_EQ(String(writer.GetError()), "JsonWriter: a value inside a struct needs a Field name first.");
    }
    {
        // 값 없이 다음 Field를 씀
        JsonWriter writer;
        writer.BeginStruct();
        writer.Field("first");
        writer.Field("second");
        EXPECT_EQ(String(writer.GetError()), "JsonWriter: field 'first' has no value.");
    }
    {
        // 루트 값을 두 개 씀
        JsonWriter writer;
        writer.Int(1, EIntWidth::Bits32, true);
        writer.Int(2, EIntWidth::Bits32, true);
        EXPECT_EQ(String(writer.GetError()), "JsonWriter: the document already has a root value.");
    }
    {
        // 루트 struct를 닫은 뒤에 또 값을 씀
        JsonWriter writer;
        writer.BeginStruct();
        writer.EndStruct();
        writer.Bool(true);
        EXPECT_EQ(String(writer.GetError()), "JsonWriter: the document already has a root value.");
    }
    {
        // BeginMapEntry 없이 맵에 값을 씀
        JsonWriter writer;
        writer.BeginMap(1);
        writer.Int(1, EIntWidth::Bits32, true);
        EXPECT_EQ(String(writer.GetError()), "JsonWriter: a value inside a map needs BeginMapEntry first.");
    }
    {
        // value 없이 맵 엔트리를 끝냄
        JsonWriter writer;
        writer.BeginMap(1);
        writer.BeginMapEntry();
        writer.Str("alice");
        writer.EndMapEntry();
        EXPECT_EQ(String(writer.GetError()), "JsonWriter: a map entry needs exactly one key and one value.");
    }
    {
        // 배열 길이보다 많이 읽음
        JsonReader reader(R"({ "numbers": [ 1 ] })");
        reader.BeginStruct();
        ASSERT_TRUE(reader.Field("numbers"));
        u64 count = 0;
        reader.BeginSeq(count);
        EXPECT_EQ(count, 1u);

        i64 value = 0;
        reader.Int(value, EIntWidth::Bits32, true);
        reader.Int(value, EIntWidth::Bits32, true);
        EXPECT_EQ(String(reader.GetError()), "JsonReader: read past the end of an array of length 1.");
    }
    {
        // 루트 값을 두 번 읽음
        JsonReader reader("5");
        i64 value = 0;
        reader.Int(value, EIntWidth::Bits32, true);
        EXPECT_FALSE(reader.HasError());
        reader.Int(value, EIntWidth::Bits32, true);
        EXPECT_EQ(String(reader.GetError()), "JsonReader: the root value was already read.");
    }
}

TEST(JsonWriterReaderTest, ToTextReportsIncompleteDocuments)
{
    {
        // 쓴 값이 없음
        const JsonWriter writer;
        const auto text = writer.ToText();
        ASSERT_TRUE(text.HasError());
        EXPECT_EQ(text.Error(), "JsonWriter: the document has no value.");
    }
    {
        // 닫지 않은 컨테이너
        JsonWriter writer;
        writer.BeginStruct();
        writer.Field("x");
        writer.Int(1, EIntWidth::Bits32, true);
        const auto text = writer.ToText();
        ASSERT_TRUE(text.HasError());
        EXPECT_EQ(text.Error(), "JsonWriter: the document has an unclosed container.");
    }
    {
        // 쓰는 중에 난 오류가 그대로 나옴
        JsonWriter writer;
        writer.BeginStruct();
        writer.Int(1, EIntWidth::Bits32, true);
        const auto text = writer.ToText();
        ASSERT_TRUE(text.HasError());
        EXPECT_EQ(text.Error(), "JsonWriter: a value inside a struct needs a Field name first.");
    }
}

TEST(JsonWriterReaderTest, TypeMismatchReportsPath)
{
    {
        const SerializeError error = ReadError<se_json_test::EditorConfig>(R"({ "window": { "width": true } })");
        EXPECT_EQ(error.path, "window.width");
        EXPECT_EQ(error.message, "JsonReader: expected an integer, got a boolean.");
    }
    {
        const SerializeError error = ReadError<se_json_test::EditorConfig>(R"({ "window": 3 })");
        EXPECT_EQ(error.path, "window");
        EXPECT_EQ(error.message, "JsonReader: expected an object, got an integer.");
    }
    {
        const SerializeError error = ReadError<se_json_test::EditorConfig>(R"({ "schemes": null })");
        EXPECT_EQ(error.path, "schemes");
        EXPECT_EQ(error.message, "JsonReader: expected an array, got null.");
    }
    {
        const SerializeError error = ReadError<se_json_test::HasItems>(R"({ "items": [ { "value": 1 }, { "value": 1.5 } ] })");
        EXPECT_EQ(error.path, "items[1].value");
    }
}

TEST(JsonWriterReaderTest, ErrorIsSticky)
{
    JsonWriter writer;
    writer.BeginStruct();
    writer.Int(1, EIntWidth::Bits32, true);

    // 오류 뒤의 노드는 문서를 바꾸지 않고, 처음 오류가 남음
    writer.Field("after");
    writer.Int(2, EIntWidth::Bits32, true);
    writer.EndStruct();

    ASSERT_TRUE(writer.HasError());
    EXPECT_EQ(String(writer.GetError()), "JsonWriter: a value inside a struct needs a Field name first.");
}
