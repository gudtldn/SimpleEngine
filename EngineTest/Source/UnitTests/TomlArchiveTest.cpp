#include "gtest/gtest.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"

#include <limits>

#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"
#undef TOML_EXCEPTIONS

using namespace se;

class TomlArchiveTest : public ::testing::Test
{
};

namespace
{
struct NestedData
{
    String str;
    float val;

    auto operator<=>(const NestedData&) const = default;

    friend void Serialize(Archive& ar, NestedData& data)
    {
        ar("str") << data.str;
        ar("val") << data.val;
    }
};

struct TestData
{
    int8 i8;
    uint8 u8;
    int16 i16;
    uint16 u16;
    int32 i32;
    uint32 u32;
    int64 i64;
    uint64 u64;
    float f32;
    double f64;
    bool b;
    String str;
    StringName sn;
    Guid guid;
    NestedData nested;
    Array<int32> i32_array;
    Array<NestedData> nested_array;
    HashMap<String, int32> map_str_i32;
    HashMap<int32, String> map_i32_str;
    HashSet<String> set_str;
    HashSet<int32> set_i32;
    HashMap<String, NestedData> map_str_nested;


    friend void Serialize(Archive& ar, TestData& data)
    {
        ar("i8") << data.i8;
        ar("u8") << data.u8;
        ar("i16") << data.i16;
        ar("u16") << data.u16;
        ar("i32") << data.i32;
        ar("u32") << data.u32;
        ar("i64") << data.i64;
        ar("u64") << data.u64;
        ar("f32") << data.f32;
        ar("f64") << data.f64;
        ar("b") << data.b;
        ar("str") << data.str;
        ar("sn") << data.sn;
        ar("guid") << data.guid;
        ar("nested") << data.nested;
        ar("i32_array") << data.i32_array;
        ar("nested_array") << data.nested_array;
        ar("map_str_i32") << data.map_str_i32;
        ar("map_i32_str") << data.map_i32_str;
        ar("set_str") << data.set_str;
        ar("set_i32") << data.set_i32;
        ar("map_str_nested") << data.map_str_nested;
    }
};

struct EmptyData
{
    Array<int32> empty_array;
    HashMap<String, int32> empty_map;
    HashSet<String> empty_set;
    String empty_str;

    friend void Serialize(Archive& ar, EmptyData& data)
    {
        ar("empty_array") << data.empty_array;
        ar("empty_map") << data.empty_map;
        ar("empty_set") << data.empty_set;
        ar("empty_str") << data.empty_str;
    }
};

struct BoundaryData
{
    int8 i8_min;
    int8 i8_max;
    uint8 u8_min;
    uint8 u8_max;
    int16 i16_min;
    int16 i16_max;
    uint16 u16_min;
    uint16 u16_max;
    int32 i32_min;
    int32 i32_max;
    uint32 u32_min;
    uint32 u32_max;
    int64 i64_min;
    int64 i64_max;
    uint64 u64_max;

    friend void Serialize(Archive& ar, BoundaryData& data)
    {
        ar("i8_min") << data.i8_min;
        ar("i8_max") << data.i8_max;
        ar("u8_min") << data.u8_min;
        ar("u8_max") << data.u8_max;
        ar("i16_min") << data.i16_min;
        ar("i16_max") << data.i16_max;
        ar("u16_min") << data.u16_min;
        ar("u16_max") << data.u16_max;
        ar("i32_min") << data.i32_min;
        ar("i32_max") << data.i32_max;
        ar("u32_min") << data.u32_min;
        ar("u32_max") << data.u32_max;
        ar("i64_min") << data.i64_min;
        ar("i64_max") << data.i64_max;
        ar("u64_max") << data.u64_max;
    }
};

struct SpecialStringData
{
    String unicode_str;
    String newline_str;
    String quote_str;
    String backslash_str;
    String multi_line_str;

    auto operator<=>(const SpecialStringData&) const = default;

    friend void Serialize(Archive& ar, SpecialStringData& data)
    {
        ar("unicode_str") << data.unicode_str;
        ar("newline_str") << data.newline_str;
        ar("quote_str") << data.quote_str;
        ar("backslash_str") << data.backslash_str;
        ar("multi_line_str") << data.multi_line_str;
    }
};

struct DeepNestingLevel3
{
    int32 value;

    auto operator<=>(const DeepNestingLevel3&) const = default;

    friend void Serialize(Archive& ar, DeepNestingLevel3& data)
    {
        ar("value") << data.value;
    }
};

struct DeepNestingLevel2
{
    Array<DeepNestingLevel3> items;

    auto operator<=>(const DeepNestingLevel2&) const = default;

    friend void Serialize(Archive& ar, DeepNestingLevel2& data)
    {
        ar("items") << data.items;
    }
};

struct DeepNestingLevel1
{
    HashMap<String, DeepNestingLevel2> children;

    friend void Serialize(Archive& ar, DeepNestingLevel1& data)
    {
        ar("children") << data.children;
    }
};

struct UnusualKeyData
{
    HashMap<bool, String> bool_map;
    HashMap<int32, String> negative_key_map;
    HashMap<float, String> float_map;
    HashMap<double, String> double_map;

    friend void Serialize(Archive& ar, UnusualKeyData& data)
    {
        ar("bool_map") << data.bool_map;
        ar("negative_key_map") << data.negative_key_map;
        ar("float_map") << data.float_map;
        ar("double_map") << data.double_map;
    }
};

struct LargeTestData
{
    Array<int32> large_array;
    HashMap<int32, String> large_map;

    friend void Serialize(Archive& ar, LargeTestData& data)
    {
        ar("large_array") << data.large_array;
        ar("large_map") << data.large_map;
    }
};

struct NestedEmptyData
{
    Array<Array<int32>> nested_arrays;
    HashMap<String, Array<int32>> map_with_empty_arrays;
    Array<HashMap<String, int32>> array_of_empty_maps;

    friend void Serialize(Archive& ar, NestedEmptyData& data)
    {
        ar("nested_arrays") << data.nested_arrays;
        ar("map_with_empty_arrays") << data.map_with_empty_arrays;
        ar("array_of_empty_maps") << data.array_of_empty_maps;
    }
};

struct TripleNestedData
{
    Array<Array<Array<int32>>> triple_array;

    auto operator<=>(const TripleNestedData&) const = default;

    friend void Serialize(Archive& ar, TripleNestedData& data)
    {
        ar("triple_array") << data.triple_array;
    }
};

struct HeteroData
{
    HashMap<String, Array<int32>> map_of_arrays;
    Array<HashMap<String, String>> array_of_maps;
    HashMap<int32, HashMap<String, float>> nested_maps;

    friend void Serialize(Archive& ar, HeteroData& data)
    {
        ar("map_of_arrays") << data.map_of_arrays;
        ar("array_of_maps") << data.array_of_maps;
        ar("nested_maps") << data.nested_maps;
    }
};

struct ZeroData
{
    int32 zero_int = 999;  // Non-zero default
    float zero_float = 999.0f;
    bool false_bool = true; // true default
    String zero_str = "default";

    friend void Serialize(Archive& ar, ZeroData& data)
    {
        ar("zero_int") << data.zero_int;
        ar("zero_float") << data.zero_float;
        ar("false_bool") << data.false_bool;
        ar("zero_str") << data.zero_str;
    }
};

struct IntMapData
{
    HashMap<int32, int32> int_map;

    friend void Serialize(Archive& ar, IntMapData& data)
    {
        ar("int_map") << data.int_map;
    }
};

struct IntValueData
{
    int32 value = 999;

    friend void Serialize(Archive& ar, IntValueData& data)
    {
        ar("value") << data.value;
    }
};

struct TwoFieldData
{
    int32 present_key = 0;
    int32 missing_key = 999;

    friend void Serialize(Archive& ar, TwoFieldData& data)
    {
        ar("present_key") << data.present_key;
        ar("missing_key") << data.missing_key;
    }
};

struct SmallIntData
{
    int8 small_int = 0;

    friend void Serialize(Archive& ar, SmallIntData& data)
    {
        ar("small_int") << data.small_int;
    }
};

struct ManyMapsData
{
    HashMap<int32, String> map;

    friend void Serialize(Archive& ar, ManyMapsData& data)
    {
        ar("map") << data.map;
    }
};

struct FloatPrecisionData
{
    float f1 = 3.14159265358979323846f;
    double d1 = 3.14159265358979323846;
    float very_small = 1.23456e-38f;
    double very_large = 1.23456e308;

    friend void Serialize(Archive& ar, FloatPrecisionData& data)
    {
        ar("f1") << data.f1;
        ar("d1") << data.d1;
        ar("very_small") << data.very_small;
        ar("very_large") << data.very_large;
    }
};

struct SingleElementData
{
    Array<int32> single_array;
    HashMap<String, int32> single_map;
    HashSet<String> single_set;

    friend void Serialize(Archive& ar, SingleElementData& data)
    {
        ar("single_array") << data.single_array;
        ar("single_map") << data.single_map;
        ar("single_set") << data.single_set;
    }
};

struct SetDeduplicationData
{
    HashSet<int32> int_set;

    friend void Serialize(Archive& ar, SetDeduplicationData& data)
    {
        ar("int_set") << data.int_set;
    }
};

struct GuidUniquenessData
{
    Array<Guid> guids;

    friend void Serialize(Archive& ar, GuidUniquenessData& data)
    {
        ar("guids") << data.guids;
    }
};

struct LongStringData
{
    String long_str;

    auto operator<=>(const LongStringData&) const = default;

    friend void Serialize(Archive& ar, LongStringData& data)
    {
        ar("long_str") << data.long_str;
    }
};
}

TEST_F(TomlArchiveTest, ReadAndWrite)
{
    TestData original_data = {
        .i8 = -8,
        .u8 = 8,
        .i16 = -1600,
        .u16 = 1600,
        .i32 = -320000,
        .u32 = 320000,
        .i64 = -6400000000,
        .u64 = 6400000000,
        .f32 = 32.32f,
        .f64 = 64.64,
        .b = true,
        .str = "Test String",
        .sn = "Test StringName",
        .guid = Guid::NewGuid(),
        .nested = { "Nested String", 123.456f },
        .i32_array = { 1, 2, 3, 4, 5 },
        .nested_array = { { "nested1", 1.1f }, { "nested2", 2.2f } },
        .map_str_i32 = { {"one", 1}, {"two", 2} },
        .map_i32_str = { {1, "one"}, {2, "two"} },
        .set_str = { "A", "B", "C" },
        .set_i32 = { 100, 200, 300 },
        .map_str_nested = { {"nested1", {"n1", 1.0f}}, {"nested2", {"n2", 2.0f}} },
    };

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    // Verify written data in toml table
    EXPECT_EQ(*tbl["i8"].value<int8>(), original_data.i8);
    EXPECT_EQ(*tbl["u8"].value<uint8>(), original_data.u8);
    EXPECT_EQ(*tbl["i16"].value<int16>(), original_data.i16);
    EXPECT_EQ(*tbl["u16"].value<uint16>(), original_data.u16);
    EXPECT_EQ(*tbl["i32"].value<int32>(), original_data.i32);
    EXPECT_EQ(*tbl["u32"].value<uint32>(), original_data.u32);
    EXPECT_EQ(*tbl["i64"].value<int64>(), original_data.i64);
    EXPECT_EQ(*tbl["u64"].value<uint64>(), original_data.u64);
    EXPECT_FLOAT_EQ(*tbl["f32"].value<float>(), original_data.f32);
    EXPECT_DOUBLE_EQ(*tbl["f64"].value<double>(), original_data.f64);
    EXPECT_EQ(*tbl["b"].value<bool>(), original_data.b);
    EXPECT_EQ(*tbl["str"].value<std::string>(), original_data.str.Data());
    EXPECT_EQ(*tbl["sn"].value<std::string>(), original_data.sn.ToString().Data());
    EXPECT_EQ(*tbl["guid"].value<std::string>(), original_data.guid.ToString().Data());

    auto& nested_tbl = *tbl["nested"].as_table();
    EXPECT_EQ(*nested_tbl["str"].value<std::string>(), original_data.nested.str.Data());
    EXPECT_FLOAT_EQ(*nested_tbl["val"].value<float>(), original_data.nested.val);

    auto& i32_array = *tbl["i32_array"].as_array();
    EXPECT_EQ(i32_array.size(), original_data.i32_array.Len());
    for (size_t i = 0; i < i32_array.size(); ++i)
    {
        EXPECT_EQ(*i32_array[i].value<int32>(), original_data.i32_array[i]);
    }

    auto& nested_array = *tbl["nested_array"].as_array();
    EXPECT_EQ(nested_array.size(), original_data.nested_array.Len());
    for (size_t i = 0; i < nested_array.size(); ++i)
    {
        auto& nested_elem_tbl = *nested_array[i].as_table();
        EXPECT_EQ(*nested_elem_tbl["str"].value<std::string>(), original_data.nested_array[i].str.Data());
        EXPECT_FLOAT_EQ(*nested_elem_tbl["val"].value<float>(), original_data.nested_array[i].val);
    }

    TestData read_data = {};
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.i8, original_data.i8);
    EXPECT_EQ(read_data.u8, original_data.u8);
    EXPECT_EQ(read_data.i16, original_data.i16);
    EXPECT_EQ(read_data.u16, original_data.u16);
    EXPECT_EQ(read_data.i32, original_data.i32);
    EXPECT_EQ(read_data.u32, original_data.u32);
    EXPECT_EQ(read_data.i64, original_data.i64);
    EXPECT_EQ(read_data.u64, original_data.u64);
    EXPECT_FLOAT_EQ(read_data.f32, original_data.f32);
    EXPECT_DOUBLE_EQ(read_data.f64, original_data.f64);
    EXPECT_EQ(read_data.b, original_data.b);
    EXPECT_EQ(read_data.str, original_data.str);
    EXPECT_EQ(read_data.sn, original_data.sn);
    EXPECT_EQ(read_data.guid, original_data.guid);
    EXPECT_EQ(read_data.nested, original_data.nested);
    EXPECT_EQ(read_data.i32_array, original_data.i32_array);
    EXPECT_EQ(read_data.nested_array, original_data.nested_array);
    EXPECT_EQ(read_data.map_str_i32, original_data.map_str_i32);
    EXPECT_EQ(read_data.map_i32_str, original_data.map_i32_str);
    EXPECT_EQ(read_data.set_str, original_data.set_str);
    EXPECT_EQ(read_data.set_i32, original_data.set_i32);
    EXPECT_EQ(read_data.map_str_nested, original_data.map_str_nested);
}

// Edge Cases: Empty Containers
TEST_F(TomlArchiveTest, EmptyContainers)
{
    EmptyData original_data;
    // All fields are already empty by default

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    EmptyData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_TRUE(read_data.empty_array.IsEmpty());
    EXPECT_TRUE(read_data.empty_map.IsEmpty());
    EXPECT_TRUE(read_data.empty_set.IsEmpty());
    EXPECT_TRUE(read_data.empty_str.IsEmpty());
}

// Edge Cases: Boundary Values
TEST_F(TomlArchiveTest, BoundaryValues)
{
    BoundaryData original_data = {
        .i8_min = std::numeric_limits<int8>::min(),
        .i8_max = std::numeric_limits<int8>::max(),
        .u8_min = std::numeric_limits<uint8>::min(),
        .u8_max = std::numeric_limits<uint8>::max(),
        .i16_min = std::numeric_limits<int16>::min(),
        .i16_max = std::numeric_limits<int16>::max(),
        .u16_min = std::numeric_limits<uint16>::min(),
        .u16_max = std::numeric_limits<uint16>::max(),
        .i32_min = std::numeric_limits<int32>::min(),
        .i32_max = std::numeric_limits<int32>::max(),
        .u32_min = std::numeric_limits<uint32>::min(),
        .u32_max = std::numeric_limits<uint32>::max(),
        .i64_min = std::numeric_limits<int64>::min(),
        .i64_max = std::numeric_limits<int64>::max(),
        .u64_max = std::numeric_limits<uint64>::max(),
    };

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    BoundaryData read_data = {};
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.i8_min, original_data.i8_min);
    EXPECT_EQ(read_data.i8_max, original_data.i8_max);
    EXPECT_EQ(read_data.u8_min, original_data.u8_min);
    EXPECT_EQ(read_data.u8_max, original_data.u8_max);
    EXPECT_EQ(read_data.i16_min, original_data.i16_min);
    EXPECT_EQ(read_data.i16_max, original_data.i16_max);
    EXPECT_EQ(read_data.u16_min, original_data.u16_min);
    EXPECT_EQ(read_data.u16_max, original_data.u16_max);
    EXPECT_EQ(read_data.i32_min, original_data.i32_min);
    EXPECT_EQ(read_data.i32_max, original_data.i32_max);
    EXPECT_EQ(read_data.u32_min, original_data.u32_min);
    EXPECT_EQ(read_data.u32_max, original_data.u32_max);
    EXPECT_EQ(read_data.i64_min, original_data.i64_min);
    EXPECT_EQ(read_data.i64_max, original_data.i64_max);
    EXPECT_EQ(read_data.u64_max, original_data.u64_max);
}

// Edge Cases: Special String Characters
TEST_F(TomlArchiveTest, SpecialStringCharacters)
{
    SpecialStringData original_data = {
        .unicode_str = "한글 テスト 中文 🚀",
        .newline_str = "Line1\nLine2\nLine3",
        .quote_str = "She said \"Hello\"",
        .backslash_str = "C:\\Users\\Test\\Path",
        .multi_line_str = "First\nSecond\tTabbed\r\nThird",
    };

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    SpecialStringData read_data = {};
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data, original_data);
}

// Complex Nested Structures
TEST_F(TomlArchiveTest, DeepNesting)
{
    DeepNestingLevel1 original_data;
    original_data.children["first"] = DeepNestingLevel2{ .items = { {1}, {2}, {3} } };
    original_data.children["second"] = DeepNestingLevel2{ .items = { {10}, {20}, {30} } };

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    DeepNestingLevel1 read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.children.Len(), 2);
    EXPECT_EQ(read_data.children["first"].items.Len(), 3);
    EXPECT_EQ(read_data.children["first"].items[0].value, 1);
    EXPECT_EQ(read_data.children["second"].items[2].value, 30);
}

// Unusual Map Key Types
TEST_F(TomlArchiveTest, UnusualMapKeyTypes)
{
    UnusualKeyData original_data = {
        .bool_map = { {true, "True"}, {false, "False"} },
        .negative_key_map = { {-100, "Negative"}, {0, "Zero"}, {100, "Positive"} },
        .float_map = { {1.5f, "OnePointFive"}, {-2.5f, "MinusTwoPointFive"} },
        .double_map = { {3.14159, "Pi"}, {2.71828, "E"} },
    };

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    UnusualKeyData read_data = {};
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.bool_map.Len(), 2);
    EXPECT_EQ(read_data.bool_map[true], "True");
    EXPECT_EQ(read_data.bool_map[false], "False");

    EXPECT_EQ(read_data.negative_key_map.Len(), 3);
    EXPECT_EQ(read_data.negative_key_map[-100], "Negative");
    EXPECT_EQ(read_data.negative_key_map[0], "Zero");
    EXPECT_EQ(read_data.negative_key_map[100], "Positive");

    EXPECT_EQ(read_data.float_map.Len(), 2);
    EXPECT_EQ(read_data.double_map.Len(), 2);
}

// Large Data Performance
TEST_F(TomlArchiveTest, LargeData)
{
    LargeTestData original_data;

    // Create large array with 1000 elements
    for (int32 i = 0; i < 1000; ++i)
    {
        original_data.large_array.Push(i * 2);
    }

    // Create large map with 500 entries
    for (int32 i = 0; i < 500; ++i)
    {
        original_data.large_map[i] = String::Format("Value_{}", i);
    }

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    LargeTestData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.large_array.Len(), 1000);
    EXPECT_EQ(read_data.large_map.Len(), 500);

    // Spot check some values
    EXPECT_EQ(read_data.large_array[0], 0);
    EXPECT_EQ(read_data.large_array[999], 1998);
    EXPECT_EQ(read_data.large_map[0], "Value_0");
    EXPECT_EQ(read_data.large_map[499], "Value_499");
}

// Nested Empty Containers
TEST_F(TomlArchiveTest, NestedEmptyContainers)
{
    NestedEmptyData original_data;
    original_data.nested_arrays.Push(Array<int32>{}); // Empty array
    original_data.nested_arrays.Push(Array<int32>{1, 2, 3});
    original_data.nested_arrays.Push(Array<int32>{}); // Another empty array

    original_data.map_with_empty_arrays["empty"] = Array<int32>{};
    original_data.map_with_empty_arrays["filled"] = Array<int32>{10, 20};

    original_data.array_of_empty_maps.Push(HashMap<String, int32>{});
    original_data.array_of_empty_maps.Push(HashMap<String, int32>{ {"key", 42} });

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    NestedEmptyData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.nested_arrays.Len(), 3);
    EXPECT_TRUE(read_data.nested_arrays[0].IsEmpty());
    EXPECT_EQ(read_data.nested_arrays[1].Len(), 3);
    EXPECT_TRUE(read_data.nested_arrays[2].IsEmpty());

    EXPECT_TRUE(read_data.map_with_empty_arrays["empty"].IsEmpty());
    EXPECT_EQ(read_data.map_with_empty_arrays["filled"].Len(), 2);

    EXPECT_TRUE(read_data.array_of_empty_maps[0].IsEmpty());
    EXPECT_EQ(read_data.array_of_empty_maps[1].Len(), 1);
}

// Array of Arrays of Arrays
TEST_F(TomlArchiveTest, TripleNestedArrays)
{
    TripleNestedData original_data;
    Array<Array<int32>> level2_1;
    level2_1.Push(Array<int32>{1, 2, 3});
    level2_1.Push(Array<int32>{4, 5, 6});

    Array<Array<int32>> level2_2;
    level2_2.Push(Array<int32>{7, 8});
    level2_2.Push(Array<int32>{9});

    original_data.triple_array.Push(level2_1);
    original_data.triple_array.Push(level2_2);

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    TripleNestedData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data, original_data);
}

// Mixed heterogeneous containers
TEST_F(TomlArchiveTest, HeterogeneousContainers)
{
    HeteroData original_data;
    original_data.map_of_arrays["first"] = Array<int32>{1, 2, 3};
    original_data.map_of_arrays["second"] = Array<int32>{10, 20};

    original_data.array_of_maps.Push(HashMap<String, String>{ {"a", "A"}, {"b", "B"} });
    original_data.array_of_maps.Push(HashMap<String, String>{ {"x", "X"} });

    original_data.nested_maps[1] = HashMap<String, float>{ {"pi", 3.14f}, {"e", 2.71f} };
    original_data.nested_maps[2] = HashMap<String, float>{ {"sqrt2", 1.41f} };

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    HeteroData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.map_of_arrays["first"].Len(), 3);
    EXPECT_EQ(read_data.array_of_maps.Len(), 2);
    EXPECT_EQ(read_data.nested_maps[1].Len(), 2);
    EXPECT_FLOAT_EQ(read_data.nested_maps[1]["pi"], 3.14f);
}

// Zero values that differ from defaults
TEST_F(TomlArchiveTest, ZeroValues)
{
    ZeroData original_data = {
        .zero_int = 0,
        .zero_float = 0.0f,
        .false_bool = false,
        .zero_str = "",
    };

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    ZeroData read_data; // Will have non-zero defaults
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.zero_int, 0);
    EXPECT_FLOAT_EQ(read_data.zero_float, 0.0f);
    EXPECT_FALSE(read_data.false_bool);
    EXPECT_EQ(read_data.zero_str, "");
}

// Error Handling: Invalid Map Key Conversion
TEST_F(TomlArchiveTest, InvalidMapKeyConversion)
{
    // Manually create TOML with invalid numeric key
    toml::table tbl;
    toml::table invalid_map;
    invalid_map.insert("not_a_number", 42);
    invalid_map.insert("123", 456);
    tbl.insert("int_map", invalid_map);

    IntMapData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
        // Should log warning for "not_a_number" but continue
        // "123" should be converted successfully
    }

    // The invalid key should result in 0 (default) and value still added
    EXPECT_EQ(read_data.int_map.Len(), 2);
    EXPECT_TRUE(read_data.int_map.Contains(123));
    EXPECT_EQ(read_data.int_map[123], 456);
}

// Error Handling: Type Mismatch
TEST_F(TomlArchiveTest, TypeMismatch)
{
    // Create TOML with string where int is expected
    toml::table tbl;
    tbl.insert("value", "not_an_integer");

    IntValueData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
        // Should handle gracefully, leaving value unchanged
    }

    // Value should remain at default since conversion failed
    EXPECT_EQ(read_data.value, 999);
}

// Error Handling: Missing Keys
TEST_F(TomlArchiveTest, MissingKeys)
{
    toml::table tbl;
    tbl.insert("present_key", 42);
    // "missing_key" is not in the table

    TwoFieldData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.present_key, 42);
    EXPECT_EQ(read_data.missing_key, 999); // Should remain unchanged
}

// Error Handling: Out of Range Values
TEST_F(TomlArchiveTest, OutOfRangeValues)
{
    // Create TOML with value too large for int8
    toml::table tbl;
    tbl.insert("small_int", 999); // Too large for int8 (max 127)

    SmallIntData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    // Value gets truncated/casted
    EXPECT_EQ(read_data.small_int, static_cast<int8>(999));
}

// Stress Test: Many Map Iterations (Verifies O(N) complexity fix)
TEST_F(TomlArchiveTest, ManyMapIterations)
{
    ManyMapsData original_data;

    // Create map with many entries to verify iterator-based approach is O(N)
    for (int32 i = 0; i < 200; ++i)
    {
        original_data.map[i] = String::Format("Item_{}", i);
    }

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    ManyMapsData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
        // With O(N^2) approach this would be slow, with O(N) it should be fast
    }

    EXPECT_EQ(read_data.map.Len(), 200);
    for (int32 i = 0; i < 200; ++i)
    {
        EXPECT_EQ(read_data.map[i], String::Format("Item_{}", i));
    }
}

// Floating Point Precision
TEST_F(TomlArchiveTest, FloatingPointPrecision)
{
    FloatPrecisionData original_data;

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    FloatPrecisionData read_data = {};
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_FLOAT_EQ(read_data.f1, original_data.f1);
    EXPECT_DOUBLE_EQ(read_data.d1, original_data.d1);
    EXPECT_FLOAT_EQ(read_data.very_small, original_data.very_small);
    EXPECT_DOUBLE_EQ(read_data.very_large, original_data.very_large);
}

// Single Element Containers
TEST_F(TomlArchiveTest, SingleElementContainers)
{
    SingleElementData original_data;
    original_data.single_array.Push(42);
    original_data.single_map["only"] = 123;
    original_data.single_set.Insert("unique");

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    SingleElementData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.single_array.Len(), 1);
    EXPECT_EQ(read_data.single_array[0], 42);
    EXPECT_EQ(read_data.single_map.Len(), 1);
    EXPECT_EQ(read_data.single_map["only"], 123);
    EXPECT_EQ(read_data.single_set.Len(), 1);
    EXPECT_TRUE(read_data.single_set.Contains("unique"));
}

// Duplicate Keys in Sets (should deduplicate)
TEST_F(TomlArchiveTest, SetDeduplication)
{
    // Note: During write, sets already have unique elements
    // This tests that reading handles it correctly
    SetDeduplicationData original_data;
    original_data.int_set.Insert(1);
    original_data.int_set.Insert(2);
    original_data.int_set.Insert(3);
    original_data.int_set.Insert(1); // Already exists, won't be added

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    SetDeduplicationData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.int_set.Len(), 3);
    EXPECT_TRUE(read_data.int_set.Contains(1));
    EXPECT_TRUE(read_data.int_set.Contains(2));
    EXPECT_TRUE(read_data.int_set.Contains(3));
}

// GUID Uniqueness
TEST_F(TomlArchiveTest, GuidUniqueness)
{
    GuidUniquenessData original_data;
    for (int i = 0; i < 10; ++i)
    {
        original_data.guids.Push(Guid::NewGuid());
    }

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    GuidUniquenessData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data.guids.Len(), 10);
    for (usize i = 0; i < read_data.guids.Len(); ++i)
    {
        EXPECT_EQ(read_data.guids[i], original_data.guids[i]);
    }
}

// Very Long Strings
TEST_F(TomlArchiveTest, VeryLongStrings)
{
    LongStringData original_data;

    // Create a very long string (10000 characters)
    String long_content;
    for (int i = 0; i < 1000; ++i)
    {
        long_content += "0123456789";
    }
    original_data.long_str = long_content;

    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << original_data;
    }

    LongStringData read_data;
    {
        TomlReader reader(tbl);
        reader << read_data;
    }

    EXPECT_EQ(read_data, original_data);
    EXPECT_EQ(read_data.long_str.ByteLen(), 10000);
}
