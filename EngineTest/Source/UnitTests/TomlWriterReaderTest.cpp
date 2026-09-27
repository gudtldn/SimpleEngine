#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

using namespace se;

// TomlWriter/TomlReader의 노드 규칙(정수 범위, f32 표현, enum 이름, 맵, Optional, Bytes, 정렬), 레거시 설정 파일 호환, 오류 경로, .meta 골든 텍스트 검증
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

/** 문자열 key 맵(테이블) 검증용 */
struct HasMap
{
    HashMap<String, i32> scores;

    [[nodiscard]] bool operator==(const HasMap&) const = default;
};

/** struct 값을 담은 테이블 맵에서 모르는 키의 경고 위치 검증용 */
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

/** struct key로 쓰는 격자 좌표 */
struct GridCell
{
    i32 x = 0;
    i32 y = 0;

    [[nodiscard]] auto operator<=>(const GridCell&) const = default;
};

/** struct key 맵 검증용 */
struct HasCellMap
{
    Map<GridCell, String> cells;

    [[nodiscard]] bool operator==(const HasCellMap&) const = default;
};

/** i64를 넘는 u64 key 맵 검증용. 그런 key는 10진 문자열로 쓰입니다. */
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

/** 맵 value의 None 검증용 */
struct HasOptionalMap
{
    HashMap<String, Optional<i32>> values;
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

/** .meta 골든 테스트에서 서브 에셋의 type으로 쓰는 메시 에셋 타입 */
struct MetaStaticMesh
{
};

/** .meta 골든 테스트에서 서브 에셋의 type으로 쓰는 머티리얼 에셋 타입 */
struct MetaMaterialInstance
{
};

/** .meta의 import_settings 값을 흉내 내는 메시 임포트 설정 */
struct MetaMeshImportSettings
{
    bool apply_transform = true;
    bool combine_meshes = true;
    f32 global_scale = 1.0f;

    [[nodiscard]] bool operator==(const MetaMeshImportSettings&) const = default;
};

/** .meta의 서브 에셋 의존성을 흉내 내는 타입 */
struct MetaDependency
{
    String source_vpath;
    Guid asset_guid;

    [[nodiscard]] bool operator==(const MetaDependency&) const = default;
};

/** .meta의 [[metadata.sub_assets]]를 흉내 내는 타입 */
struct MetaSubAsset
{
    String name;
    Guid guid;
    TypeId type;
    Array<MetaDependency> dependencies;

    [[nodiscard]] bool operator==(const MetaSubAsset&) const = default;
};

/** .meta의 [metadata]를 흉내 내는 타입 */
struct MetaMetadata
{
    Guid guid;
    ContentHash source_hash;
    u64 source_mtime = 0;
    u64 source_size = 0;
    u32 cache_version = 0;
    ContentHash settings_hash;
    Array<MetaSubAsset> sub_assets;

    [[nodiscard]] bool operator==(const MetaMetadata&) const = default;
};

/** .meta의 processor_stack 원소를 흉내 내는 타입 */
struct MetaProcessorEntry
{
    TypeId processor_type;
    bool enabled = true;

    [[nodiscard]] bool operator==(const MetaProcessorEntry&) const = default;
};

/** .meta 파일 하나와 구조가 같은 루트 타입. 타입 이름이 key인 맵, 중첩 배열을 가진 struct 배열, u64, hex 문자열을 담습니다. */
struct MetaFile
{
    MetaMetadata metadata;
    HashMap<TypeId, MetaMeshImportSettings> import_settings;
    Array<MetaProcessorEntry> processor_stack;

    [[nodiscard]] bool operator==(const MetaFile&) const = default;
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
SE_DECLARE_REFLECTION(se_toml_test::HasItemMap)
SE_DECLARE_REFLECTION(se_toml_test::HasIdMap)
SE_DECLARE_REFLECTION(se_toml_test::HasModeMap)
SE_DECLARE_REFLECTION(se_toml_test::GridCell)
SE_DECLARE_REFLECTION(se_toml_test::HasCellMap)
SE_DECLARE_REFLECTION(se_toml_test::HasU64Map)
SE_DECLARE_REFLECTION(se_toml_test::HasOptional)
SE_DECLARE_REFLECTION(se_toml_test::HasDefaultSome)
SE_DECLARE_REFLECTION(se_toml_test::HasOptionalArray)
SE_DECLARE_REFLECTION(se_toml_test::HasOptionalMap)
SE_DECLARE_REFLECTION(se_toml_test::HasNestedOptional)
SE_DECLARE_REFLECTION(se_toml_test::HasSets)
SE_DECLARE_REFLECTION(se_toml_test::MetaStaticMesh)
SE_DECLARE_REFLECTION(se_toml_test::MetaMaterialInstance)
SE_DECLARE_REFLECTION(se_toml_test::MetaMeshImportSettings)
SE_DECLARE_REFLECTION(se_toml_test::MetaDependency)
SE_DECLARE_REFLECTION(se_toml_test::MetaSubAsset)
SE_DECLARE_REFLECTION(se_toml_test::MetaMetadata)
SE_DECLARE_REFLECTION(se_toml_test::MetaProcessorEntry)
SE_DECLARE_REFLECTION(se_toml_test::MetaFile)

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

SE_REFLECT_BEGIN(se_toml_test::HasItemMap)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasIdMap)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasModeMap)
    SE_FIELD(modes)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::GridCell)
    SE_FIELD(x)
    SE_FIELD(y)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasCellMap)
    SE_FIELD(cells)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasU64Map)
    SE_FIELD(values)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasOptional)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasDefaultSome)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasOptionalArray)
    SE_FIELD(values)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasOptionalMap)
    SE_FIELD(values)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasNestedOptional)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::HasSets)
    SE_FIELD(ints)
    SE_FIELD(floats)
    SE_FIELD(flags)
    SE_FIELD(names)
    SE_FIELD(modes)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaStaticMesh)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaMaterialInstance)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaMeshImportSettings)
    SE_FIELD(apply_transform)
    SE_FIELD(combine_meshes)
    SE_FIELD(global_scale)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaDependency)
    SE_FIELD(source_vpath)
    SE_FIELD(asset_guid)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaSubAsset)
    SE_FIELD(name)
    SE_FIELD(guid)
    SE_FIELD(type)
    SE_FIELD(dependencies)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaMetadata)
    SE_FIELD(guid)
    SE_FIELD(source_hash)
    SE_FIELD(source_mtime)
    SE_FIELD(source_size)
    SE_FIELD(cache_version)
    SE_FIELD(settings_hash)
    SE_FIELD(sub_assets)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaProcessorEntry)
    SE_FIELD(processor_type)
    SE_FIELD(enabled)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_toml_test::MetaFile)
    SE_FIELD(metadata)
    SE_FIELD(import_settings)
    SE_FIELD(processor_stack)
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


// --- 맵 ---

TEST(TomlWriterReaderTest, StringKeyMapIsWrittenAsTable)
{
    using namespace se_toml_test;

    const HasMap original{ .scores = { { "alice", 95 }, { "bob", 87 } } };
    const toml::table table = WriteToTable(original);

    ASSERT_TRUE(table["scores"].is_table());
    EXPECT_EQ(table["scores"]["alice"].value_exact<i64>(), 95);
    EXPECT_EQ(table["scores"]["bob"].value_exact<i64>(), 87);

    TomlReader reader(table);
    HasMap result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
}

TEST(TomlWriterReaderTest, EnumKeyMapIsWrittenAsTableOfNames)
{
    using namespace se_toml_test;

    const HasModeMap original{ .modes = { { EPresentMode::VSync, 60 }, { EPresentMode::Immediate, 0 } } };
    const toml::table table = WriteToTable(original);

    EXPECT_EQ(table["modes"]["VSync"].value_exact<i64>(), 60);
    EXPECT_EQ(table["modes"]["Immediate"].value_exact<i64>(), 0);

    // 테이블의 키를 enum 이름으로 읽음
    TomlReader reader(table);
    HasModeMap result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
}

TEST(TomlWriterReaderTest, NonStringKeyMapIsWrittenAsSortedPairs)
{
    using namespace se_toml_test;

    const HasIdMap original{
        .items = { { 10, Item{ .value = 100 } }, { -2, Item{ .value = -20 } }, { 3, Item{ .value = 30 } } },
    };
    const toml::table table = WriteToTable(original);

    // key 순서로 정렬한 [key, value] 쌍 배열
    EXPECT_EQ(ToText(table), "items = [ [ -2, { value = -20 } ], [ 3, { value = 30 } ], [ 10, { value = 100 } ] ]");

    TomlReader reader(table);
    HasIdMap result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
}

TEST(TomlWriterReaderTest, StructKeyMapIsSortedByText)
{
    using namespace se_toml_test;

    const HasCellMap original{
        .cells = { { GridCell{ .x = 9, .y = 0 }, "nine" }, { GridCell{ .x = 10, .y = 0 }, "ten" } },
    };
    const toml::table table = WriteToTable(original);

    // struct key는 테이블이라 TOML 텍스트로 비교하므로 "x = 10"이 "x = 9"보다 먼저
    EXPECT_EQ(ToText(table), "cells = [ [ { x = 10, y = 0 }, 'ten' ], [ { x = 9, y = 0 }, 'nine' ] ]");

    TomlReader reader(table);
    HasCellMap result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
}

TEST(TomlWriterReaderTest, U64KeysAboveI64AreTableKeys)
{
    using namespace se_toml_test;

    // i64를 넘는 u64는 10진 문자열로 쓰이므로 key가 모두 그렇다면 테이블이 되고, 테이블의 키를 정수로 읽음
    const HasU64Map original{ .values = { { std::numeric_limits<u64>::max(), 1 } } };
    const toml::table table = WriteToTable(original);
    EXPECT_EQ(table["values"]["18446744073709551615"].value_exact<i64>(), 1);

    TomlReader reader(table);
    HasU64Map result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
}

TEST(TomlWriterReaderTest, EmptyMapIsWrittenAsEmptyTable)
{
    using namespace se_toml_test;

    const toml::table table = WriteToTable(HasIdMap{});
    ASSERT_TRUE(table["items"].is_table());
    EXPECT_TRUE(table["items"].as_table()->empty());

    // 빈 맵은 빈 테이블과 빈 배열을 모두 받고, 기존 엔트리는 지움
    for (const std::string_view text : { "items = {}", "items = []" })
    {
        const toml::table parsed = ParseToml(text);
        TomlReader reader(parsed);
        HasIdMap result{ .items = { { 1, Item{ .value = 1 } } } };
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue()) << text;
        EXPECT_TRUE(result.items.IsEmpty()) << text;
    }
}

TEST(TomlWriterReaderTest, MapIsReadFromTableOrPairs)
{
    using namespace se_toml_test;

    const HasMap expected{ .scores = { { "alice", 1 }, { "bob", 2 } } };
    for (const std::string_view text : { "scores = { alice = 1, bob = 2 }", "scores = [ [ 'bob', 2 ], [ 'alice', 1 ] ]" })
    {
        const toml::table table = ParseToml(text);
        TomlReader reader(table);
        HasMap result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue()) << text;
        EXPECT_EQ(result, expected) << text;
    }
}

TEST(TomlWriterReaderTest, MapReadErrors)
{
    struct Case
    {
        std::string_view text;
        const char* path;
        const char* message;
    };
    const Case cases[] = {
        { "scores = 5", "scores", "TomlReader: expected a table or an array, got an integer." },
        { "scores = { alice = 'x' }", "scores[0].value", "TomlReader: expected an integer, got a string." },
        { "scores = [ 'alice' ]", "scores[0].key", "TomlReader: expected a [key, value] array, got a string." },
        { "scores = [ [ 'alice' ] ]", "scores[0].key", "TomlReader: expected a [key, value] array, got an array of length 1." },
        { "scores = [ [ 1, 2 ] ]", "scores[0].key", "TomlReader: expected a string, got an integer." },
    };

    for (const Case& c : cases)
    {
        const toml::table table = ParseToml(c.text);
        TomlReader reader(table);
        se_toml_test::HasMap result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError()) << c.text;
        EXPECT_EQ(read_result.Error().path, c.path) << c.text;
        EXPECT_EQ(read_result.Error().message, c.message) << c.text;
    }
}

TEST(TomlWriterReaderTest, DuplicateTableKeyIsError)
{
    // 서로 다른 key가 같은 문자열로 쓰이면 테이블에서 한쪽이 사라지므로 오류
    toml::table table;
    TomlWriter writer(table);
    writer.BeginStruct();
    writer.Field("names");
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
    EXPECT_EQ(String(writer.GetError()), "TomlWriter: map key 'same' is written twice.");
}


// --- 정렬 ---

TEST(TomlWriterReaderTest, UnorderedSequencesAreWrittenSorted)
{
    using namespace se_toml_test;

    const HasSets original{
        .ints = { 30, -5, 7, 0 },
        .floats = { 2.5, -1.0, 0.5 },
        .flags = { true, false },
        .names = { "b", "a", "B" },
        .modes = { EPresentMode::VSync, static_cast<EPresentMode>(7), EPresentMode::Mailbox },
    };
    const toml::table table = WriteToTable(original);

    // 정수와 실수는 값, 문자열은 사전순, bool은 false가 먼저. 이름 없는 enum 값(7)은 정수라 종류 순서로 문자열 뒤
    EXPECT_EQ(ToText(table),
        "flags = [ false, true ]\n"
        "floats = [ -1.0, 0.5, 2.5 ]\n"
        "ints = [ -5, 0, 7, 30 ]\n"
        "modes = [ 'Mailbox', 'VSync', 7 ]\n"
        "names = [ 'B', 'a', 'b' ]");

    TomlReader reader(table);
    HasSets result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
}

TEST(TomlWriterReaderTest, SameValueGivesSameText)
{
    using namespace se_toml_test;

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

    EXPECT_EQ(ToText(WriteToTable(forward_map)), ToText(WriteToTable(backward_map)));
    EXPECT_EQ(ToText(WriteToTable(forward_sets)), ToText(WriteToTable(backward_sets)));
}


// --- Optional ---

TEST(TomlWriterReaderTest, NoneFieldIsOmitted)
{
    using namespace se_toml_test;

    // None은 키를 쓰지 않고, Some은 값을 그대로 씀
    EXPECT_TRUE(WriteToTable(HasOptional{}).empty());
    EXPECT_EQ(WriteToTable(HasOptional{ .value = 3 })["value"].value_exact<i64>(), 3);

    for (const HasOptional& original : { HasOptional{}, HasOptional{ .value = 3 } })
    {
        const toml::table table = WriteToTable(original);
        TomlReader reader(table);
        HasOptional result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result, original);
    }
}

TEST(TomlWriterReaderTest, MissingOptionalFieldIsReadAsNone)
{
    using namespace se_toml_test;

    const toml::table table = WriteToTable(HasDefaultSome{ .value = NullOpt });
    EXPECT_TRUE(table.empty());

    // 기본값이 Some이어도 키가 없으면 None
    TomlReader reader(table);
    HasDefaultSome result;
    ASSERT_TRUE(result.value.HasValue());
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_FALSE(result.value.HasValue());
}


// --- Bytes ---

TEST(TomlWriterReaderTest, BytesAreWrittenAsBase64)
{
    const u8 original[] = { 'M', 'a', 'n', 0xFF };

    toml::table table;
    TomlWriter writer(table);
    writer.BeginStruct();
    writer.Field("blob");
    writer.Bytes(original, sizeof(original));
    writer.EndStruct();
    ASSERT_FALSE(writer.HasError());
    EXPECT_EQ(table["blob"].value_exact<std::string>(), "TWFu/w==");

    TomlReader reader(table);
    reader.BeginStruct();
    ASSERT_TRUE(reader.Field("blob"));
    u8 result[4] = {};
    reader.Bytes(result, sizeof(result));
    reader.EndStruct();
    ASSERT_FALSE(reader.HasError());
    EXPECT_TRUE(std::ranges::equal(result, original));
}

TEST(TomlWriterReaderTest, BytesReadErrors)
{
    struct Case
    {
        std::string_view text;
        const char* message;
    };
    const Case cases[] = {
        { "blob = 'TWF'", "TomlReader: invalid base64 string." },
        { "blob = 'TWFu/w=='", "TomlReader: expected 3 bytes, got 4 bytes of base64 data." },
        { "blob = 3", "TomlReader: expected a base64 string, got an integer." },
    };

    for (const Case& c : cases)
    {
        const toml::table table = ParseToml(c.text);
        TomlReader reader(table);
        reader.BeginStruct();
        ASSERT_TRUE(reader.Field("blob")) << c.text;
        u8 result[3] = {};
        reader.Bytes(result, sizeof(result));
        EXPECT_EQ(String(reader.GetError()), c.message) << c.text;
    }
}


// --- 레거시 호환과 필드 누락 ---

TEST(TomlWriterReaderTest, ReadsLegacyConfigText)
{
    using namespace se_toml_test;

    // 레거시 직렬화가 쓴 설정 파일의 모양: f32는 넓힌 값, enum은 정수, 모르는 키와 섹션이 있음
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

TEST(TomlWriterReaderTest, UnknownKeyInMapValueHasEntryPath)
{
    {
        // 테이블 맵의 value는 "맵.키"
        const toml::table table = ParseToml("[items.alice]\nvalue = 1\nextra = true");
        TomlReader reader(table);
        se_toml_test::HasItemMap result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

        const ArrayView<const String> warnings = reader.GetWarnings();
        ASSERT_EQ(warnings.Len(), 1u);
        EXPECT_EQ(warnings[0], "TomlReader: unknown key 'items.alice.extra' is ignored.");
    }
    {
        // 쌍 배열 맵의 value는 "맵[엔트리 번호][1]"
        const toml::table table = ParseToml("items = [ [ 7, { value = 1, extra = true } ] ]");
        TomlReader reader(table);
        se_toml_test::HasIdMap result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

        const ArrayView<const String> warnings = reader.GetWarnings();
        ASSERT_EQ(warnings.Len(), 1u);
        EXPECT_EQ(warnings[0], "TomlReader: unknown key 'items[0][1].extra' is ignored.");
    }
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

TEST(TomlWriterReaderTest, NoneOutsideStructFieldIsError)
{
    using namespace se_toml_test;

    const auto write_error = [](const auto& value) -> SerializeError
    {
        toml::table table;
        TomlWriter writer(table);
        const auto write_result = serde::Serialize(writer, value);
        return write_result.HasError() ? write_result.Error() : SerializeError{ .message = "no error" };
    };

    // 시퀀스 원소와 맵 value의 None은 생략하면 그 자리가 사라짐
    const SerializeError in_array = write_error(HasOptionalArray{ .values = { 1, NullOpt } });
    EXPECT_EQ(in_array.path, "values[1]");
    EXPECT_EQ(in_array.message, "TomlWriter: None can only be written as a struct field, by omitting its key.");

    const SerializeError in_map = write_error(HasOptionalMap{ .values = { { "a", NullOpt } } });
    EXPECT_EQ(in_map.path, "values[0].value");
    EXPECT_EQ(in_map.message, "TomlWriter: None can only be written as a struct field, by omitting its key.");

    // Some(None)은 키를 생략하면 바깥 None으로 읽힘
    HasNestedOptional nested_value;
    nested_value.value.Emplace();
    const SerializeError nested = write_error(nested_value);
    EXPECT_EQ(nested.path, "value");
    EXPECT_EQ(nested.message, "TomlWriter: None inside another Optional cannot be written because the omitted key reads back as the outer None.");

    {
        // 맵 key의 None
        toml::table table;
        TomlWriter writer(table);
        writer.BeginStruct();
        writer.Field("values");
        writer.BeginMap(1);
        writer.BeginMapEntry();
        writer.Present(false);
        EXPECT_EQ(String(writer.GetError()), "TomlWriter: None can only be written as a struct field, by omitting its key.");
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
        // BeginMapEntry 없이 맵에 값을 씀
        toml::table table;
        TomlWriter writer(table);
        writer.BeginStruct();
        writer.Field("scores");
        writer.BeginMap(1);
        writer.Int(1, EIntWidth::Bits32, true);
        EXPECT_EQ(String(writer.GetError()), "TomlWriter: a value inside a map needs BeginMapEntry first.");
    }
    {
        // value 없이 맵 엔트리를 끝냄
        toml::table table;
        TomlWriter writer(table);
        writer.BeginStruct();
        writer.Field("scores");
        writer.BeginMap(1);
        writer.BeginMapEntry();
        writer.Str("alice");
        writer.EndMapEntry();
        EXPECT_EQ(String(writer.GetError()), "TomlWriter: a map entry needs exactly one key and one value.");
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


// --- .meta 골든 ---

namespace
{
/** MakeMetaFile()을 쓴 텍스트. 실제 .meta 파일(EngineCore/Assets/Cube.obj.meta)과 배치가 같습니다. */
constexpr std::string_view META_FILE_GOLDEN_TEXT =
    "processor_stack = []\n"
    "\n"
    "[import_settings.'se_toml_test::MetaMeshImportSettings']\n"
    "apply_transform = true\n"
    "combine_meshes = true\n"
    "global_scale = 1.0\n"
    "\n"
    "[metadata]\n"
    "cache_version = 1\n"
    "guid = '0dd9f95f-f684-4b35-8f42-e2ce2d9f52ab'\n"
    "settings_hash = '82a967f4e418eb518b3e599736e29c936cf3452045c20b375a5d854590bc417a'\n"
    "source_hash = '44cef8efa27cc51242067df382cc419a7b50e43e866ef5b467f7efa46eaddeeb'\n"
    "source_mtime = 1775110252411093200\n"
    "source_size = 945\n"
    "\n"
    "    [[metadata.sub_assets]]\n"
    "    dependencies = []\n"
    "    guid = 'df89d951-dc57-4bc7-90d8-df460daea1b8'\n"
    "    name = 'Cube'\n"
    "    type = 'se_toml_test::MetaStaticMesh'\n"
    "\n"
    "    [[metadata.sub_assets]]\n"
    "    guid = '86cf11d2-ee50-46c9-ac18-76ca5172c7c1'\n"
    "    name = 'Material_DefaultMaterial'\n"
    "    type = 'se_toml_test::MetaMaterialInstance'\n"
    "\n"
    "        [[metadata.sub_assets.dependencies]]\n"
    "        asset_guid = '3f2a1c9e-8b7d-4e6f-a5c4-b3d2e1f0a9b8'\n"
    "        source_vpath = 'Assets://Textures/Wood_Diffuse.png'";

/** .meta 파일 하나를 흉내 낸 값. 서브 에셋 하나는 의존성이 없고, 하나는 의존성이 하나 있습니다. */
[[nodiscard]] se_toml_test::MetaFile MakeMetaFile()
{
    using namespace se_toml_test;

    return MetaFile{
        .metadata = MetaMetadata{
            .guid = Guid::FromString("0dd9f95f-f684-4b35-8f42-e2ce2d9f52ab"),
            .source_hash = ContentHash::FromHex("44cef8efa27cc51242067df382cc419a7b50e43e866ef5b467f7efa46eaddeeb"),
            .source_mtime = 1775110252411093200,
            .source_size = 945,
            .cache_version = 1,
            .settings_hash = ContentHash::FromHex("82a967f4e418eb518b3e599736e29c936cf3452045c20b375a5d854590bc417a"),
            .sub_assets = {
                MetaSubAsset{
                    .name = "Cube",
                    .guid = Guid::FromString("df89d951-dc57-4bc7-90d8-df460daea1b8"),
                    .type = TypeId::Of<MetaStaticMesh>(),
                },
                MetaSubAsset{
                    .name = "Material_DefaultMaterial",
                    .guid = Guid::FromString("86cf11d2-ee50-46c9-ac18-76ca5172c7c1"),
                    .type = TypeId::Of<MetaMaterialInstance>(),
                    .dependencies = {
                        MetaDependency{
                            .source_vpath = "Assets://Textures/Wood_Diffuse.png",
                            .asset_guid = Guid::FromString("3f2a1c9e-8b7d-4e6f-a5c4-b3d2e1f0a9b8"),
                        },
                    },
                },
            },
        },
        .import_settings = { { TypeId::Of<MetaMeshImportSettings>(), MetaMeshImportSettings{} } },
    };
}
} // namespace

TEST(TomlWriterReaderTest, MetaFileMatchesGoldenText)
{
    using namespace se_toml_test;

    const MetaFile original = MakeMetaFile();
    EXPECT_EQ(ToText(WriteToTable(original)), META_FILE_GOLDEN_TEXT);

    // 골든 텍스트를 읽으면 같은 값이고, 타입에 없는 키가 없어 경고가 0건
    const toml::table table = ParseToml(META_FILE_GOLDEN_TEXT);
    TomlReader reader(table);
    MetaFile result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, original);
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
}
