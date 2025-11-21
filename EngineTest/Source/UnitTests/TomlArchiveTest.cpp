#include "gtest/gtest.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"
#undef TOML_EXCEPTIONS

using namespace se;
using namespace se::core;

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
        .nested_array = { { "nested1", 1.1f }, { "nested2", 2.2f } }
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
}
