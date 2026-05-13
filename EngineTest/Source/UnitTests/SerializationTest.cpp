#include "gtest/gtest.h"

#include <limits>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Container/Set.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"

using namespace se;

class SerializationTest : public ::testing::Test
{
};

TEST_F(SerializationTest, ReadAndWritePrimitives)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    // 원본 데이터
    i32 original_int = -12345;
    f32 original_float = 3.14159f;
    bool original_bool = true;
    f64 original_double = 1.23456789;

    // 쓰기
    writer << original_int << original_float << original_bool << original_double;

    constexpr usize EXPECTED_SIZE = sizeof(original_int) + sizeof(original_float) + sizeof(original_bool) + sizeof(original_double);
    EXPECT_EQ(buffer.Len(), EXPECTED_SIZE);
    EXPECT_EQ(writer.Tell(), EXPECTED_SIZE);

    // 읽기
    MemoryReader reader(buffer);
    i32 read_int = 0;
    f32 read_float = 0.0f;
    bool read_bool = false;
    f64 read_double = 0.0;

    reader << read_int << read_float << read_bool << read_double;

    // 검증
    EXPECT_EQ(read_int, original_int);
    EXPECT_FLOAT_EQ(read_float, original_float);
    EXPECT_EQ(read_bool, original_bool);
    EXPECT_DOUBLE_EQ(read_double, original_double);
    EXPECT_EQ(reader.Tell(), EXPECTED_SIZE);
}

TEST_F(SerializationTest, SeekAndTell)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    i32 val1 = 100;
    i32 val2 = 200;
    i32 val3 = 300; // 덮어쓸 값

    writer << val1;
    writer << val2;

    EXPECT_EQ(writer.Tell(), sizeof(val1) + sizeof(val2));

    // 맨 앞으로 이동해서 덮어쓰기
    writer.Seek(0);
    EXPECT_EQ(writer.Tell(), 0);
    writer << val3;
    EXPECT_EQ(writer.Tell(), sizeof(val3));

    // 읽기 및 검증
    MemoryReader reader(buffer);
    i32 read_val1 = 0;
    i32 read_val2 = 0;

    reader << read_val1;
    EXPECT_EQ(read_val1, val3); // val1은 val3로 덮어씌워졌어야 함

    reader << read_val2;
    EXPECT_EQ(read_val2, val2);

    // Reader에서 Seek 테스트
    reader.Seek(0);
    EXPECT_EQ(reader.Tell(), 0);
    reader << read_val1;
    EXPECT_EQ(read_val1, val3);
}

namespace
{
struct TestStruct
{
    int a;
    f32 b;
    bool c;

    auto operator<=>(const TestStruct&) const = default;

    friend void Serialize(Archive& ar, TestStruct& value)
    {
        ar("a") << value.a;
        ar("b") << value.b;
        ar("c") << value.c;
    }
};
}

TEST_F(SerializationTest, SerializeStruct)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    TestStruct original_struct = { -1, 123.456f, true };
    TestStruct read_struct = { 0, 0.0f, false };

    // 쓰기
    writer << original_struct;

    // 읽기
    MemoryReader reader(buffer);
    reader << read_struct;

    // 검증
    EXPECT_EQ(read_struct, original_struct);
}

TEST_F(SerializationTest, SerializeArray)
{
    // 1. Trivial type (int)
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        Array<i32> original_array = { 1, 2, 3, 4, 5 };
        Array<i32> read_array;
        writer << original_array;
        MemoryReader reader(buffer);
        reader << read_array;
        EXPECT_EQ(read_array, original_array);
    }

    // 2. Non-trivial type (String)
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        Array<String> original_array = { "Hello", "World", "!", "TestString" };
        Array<String> read_array;
        writer << original_array;
        MemoryReader reader(buffer);
        reader << read_array;
        EXPECT_EQ(read_array, original_array);
    }

    // 3. Empty array
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        Array<i32> original_array;
        Array<i32> read_array = { 1, 2, 3 }; // Not empty initially
        writer << original_array;
        MemoryReader reader(buffer);
        reader << read_array;
        EXPECT_TRUE(read_array.IsEmpty());
    }
}

TEST_F(SerializationTest, SerializeHashMap)
{
    // 1. Trivial Key/Value
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        HashMap<i32, f32> original_map = { { 1, 1.1f }, { 2, 2.2f }, { 3, 3.3f } };
        HashMap<i32, f32> read_map;
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_EQ(read_map, original_map);
    }

    // 2. Non-trivial Value
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        HashMap<i32, String> original_map = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };
        HashMap<i32, String> read_map;
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_EQ(read_map, original_map);
    }

    // 3. Empty map
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        HashMap<i32, i32> original_map;
        HashMap<i32, i32> read_map = { { 1, 1 } };
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_TRUE(read_map.IsEmpty());
    }
}

TEST_F(SerializationTest, SerializeHashSet)
{
    // 1. Trivial Key
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        HashSet<i32> original_set = { 1, 2, 3, 100, 200 };
        HashSet<i32> read_set;
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_EQ(read_set, original_set);
    }

    // 2. Non-trivial Key
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        HashSet<String> original_set = { "A", "B", "C", "Hello" };
        HashSet<String> read_set;
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_EQ(read_set, original_set);
    }

    // 3. Empty set
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        HashSet<i32> original_set;
        HashSet<i32> read_set = { 1, 2, 3 };
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_TRUE(read_set.IsEmpty());
    }
}

TEST_F(SerializationTest, SerializeMap)
{
    // Map is an alias for sorted map, test it as well
    // 1. Trivial Key/Value
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        Map<i32, f32> original_map = { { 1, 1.1f }, { 3, 3.3f }, { 2, 2.2f } };
        Map<i32, f32> read_map;
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_EQ(read_map, original_map);
    }

    // 2. Non-trivial Value
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        Map<String, i32> original_map = { { "One", 1 }, { "Two", 2 }, { "Three", 3 } };
        Map<String, i32> read_map;
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_EQ(read_map, original_map);
    }
}

TEST_F(SerializationTest, SerializeSet)
{
    // Set is an alias for sorted set
    // 1. Trivial Key
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        Set<i32> original_set = { 1, 100, 3, 200, 2 };
        Set<i32> read_set;
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_EQ(read_set, original_set);
    }

    // 2. Non-trivial Key
    {
        Array<u8> buffer;
        MemoryWriter writer(buffer);
        Set<String> original_set = { "Hello", "A", "World", "C" };
        Set<String> read_set;
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_EQ(read_set, original_set);
    }
}

// Boundary Values Test
TEST_F(SerializationTest, BoundaryValues)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    i8 i8_min = std::numeric_limits<i8>::min();
    i8 i8_max = std::numeric_limits<i8>::max();
    u64 u64_max = std::numeric_limits<u64>::max();
    f32 f_min = std::numeric_limits<f32>::lowest();
    f32 f_max = std::numeric_limits<f32>::max();
    f64 d_min = std::numeric_limits<f64>::lowest();
    f64 d_max = std::numeric_limits<f64>::max();

    writer << i8_min << i8_max << u64_max << f_min << f_max << d_min << d_max;

    MemoryReader reader(buffer);
    i8 r_i8_min = 0, r_i8_max = 0;
    u64 r_u64_max = 0;
    f32 r_f_min = 0.0f, r_f_max = 0.0f;
    f64 r_d_min = 0.0, r_d_max = 0.0;

    reader << r_i8_min << r_i8_max << r_u64_max << r_f_min << r_f_max << r_d_min << r_d_max;

    EXPECT_EQ(r_i8_min, i8_min);
    EXPECT_EQ(r_i8_max, i8_max);
    EXPECT_EQ(r_u64_max, u64_max);
    EXPECT_FLOAT_EQ(r_f_min, f_min);
    EXPECT_FLOAT_EQ(r_f_max, f_max);
    EXPECT_DOUBLE_EQ(r_d_min, d_min);
    EXPECT_DOUBLE_EQ(r_d_max, d_max);
}

namespace
{
struct Inner
{
    i32 value;
    auto operator<=>(const Inner&) const = default;

    friend void Serialize(Archive& ar, Inner& data)
    {
        ar("value") << data.value;
    }
};

struct Middle
{
    Array<Inner> inners;
    auto operator<=>(const Middle&) const = default;

    friend void Serialize(Archive& ar, Middle& data)
    {
        ar("inners") << data.inners;
    }
};

struct Outer
{
    HashMap<i32, Middle> middles;

    friend void Serialize(Archive& ar, Outer& data)
    {
        ar("middles") << data.middles;
    }
};

}

// Deep Nesting Test
TEST_F(SerializationTest, DeepNesting)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    Outer original;
    original.middles[1].inners.Push({10});
    original.middles[1].inners.Push({20});
    original.middles[2].inners.Push({30});

    writer << original;

    MemoryReader reader(buffer);
    Outer read;
    reader << read;

    EXPECT_EQ(read.middles.Len(), 2);
    EXPECT_EQ(read.middles[1].inners.Len(), 2);
    EXPECT_EQ(read.middles[1].inners[0].value, 10);
    EXPECT_EQ(read.middles[2].inners[0].value, 30);
}

// Large Data Performance Test
TEST_F(SerializationTest, LargeData)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    Array<i32> large_array;
    for (i32 i = 0; i < 10000; ++i)
    {
        large_array.Push(i * 3);
    }

    writer << large_array;

    MemoryReader reader(buffer);
    Array<i32> read_array;
    reader << read_array;

    EXPECT_EQ(read_array.Len(), 10000);
    EXPECT_EQ(read_array[0], 0);
    EXPECT_EQ(read_array[9999], 29997);
}

// Nested Empty Containers
TEST_F(SerializationTest, NestedEmptyContainers)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    Array<Array<i32>> nested_arrays;
    nested_arrays.Push(Array<i32>{}); // Empty
    nested_arrays.Push(Array<i32>{1, 2, 3}); // Filled
    nested_arrays.Push(Array<i32>{}); // Empty again

    writer << nested_arrays;

    MemoryReader reader(buffer);
    Array<Array<i32>> read_arrays;
    reader << read_arrays;

    EXPECT_EQ(read_arrays.Len(), 3);
    EXPECT_TRUE(read_arrays[0].IsEmpty());
    EXPECT_EQ(read_arrays[1].Len(), 3);
    EXPECT_TRUE(read_arrays[2].IsEmpty());
}

// Multiple Sequential Reads/Writes
TEST_F(SerializationTest, MultipleSequentialOperations)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    // Write multiple values consecutively
    i32 v1 = 100, v2 = 200, v3 = 300;
    String s1 = "First", s2 = "Second";
    bool b1 = true, b2 = false;

    writer << v1 << s1 << b1 << v2 << s2 << b2 << v3;

    MemoryReader reader(buffer);
    i32 r1 = 0, r2 = 0, r3 = 0;
    String rs1, rs2;
    bool rb1 = false, rb2 = true;

    reader << r1 << rs1 << rb1 << r2 << rs2 << rb2 << r3;

    EXPECT_EQ(r1, v1);
    EXPECT_EQ(r2, v2);
    EXPECT_EQ(r3, v3);
    EXPECT_EQ(rs1, s1);
    EXPECT_EQ(rs2, s2);
    EXPECT_EQ(rb1, b1);
    EXPECT_EQ(rb2, b2);
}

// Mixed Container Types
TEST_F(SerializationTest, MixedContainers)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    HashMap<String, Array<i32>> map_of_arrays;
    map_of_arrays["first"] = Array<i32>{1, 2, 3};
    map_of_arrays["second"] = Array<i32>{10, 20};

    Array<HashMap<String, String>> array_of_maps;
    array_of_maps.Push(HashMap<String, String>{ {"a", "A"}, {"b", "B"} });
    array_of_maps.Push(HashMap<String, String>{ {"x", "X"} });

    writer << map_of_arrays << array_of_maps;

    MemoryReader reader(buffer);
    HashMap<String, Array<i32>> read_map_of_arrays;
    Array<HashMap<String, String>> read_array_of_maps;

    reader << read_map_of_arrays << read_array_of_maps;

    EXPECT_EQ(read_map_of_arrays["first"].Len(), 3);
    EXPECT_EQ(read_map_of_arrays["second"][1], 20);
    EXPECT_EQ(read_array_of_maps.Len(), 2);
    EXPECT_EQ(read_array_of_maps[0]["a"], "A");
}

// Zero Values Test
TEST_F(SerializationTest, ZeroValues)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    i32 zero_int = 0;
    f32 zero_float = 0.0f;
    String empty_str = "";
    bool false_bool = false;

    writer << zero_int << zero_float << empty_str << false_bool;

    MemoryReader reader(buffer);
    i32 r_zero_int = 999;
    f32 r_zero_float = 999.0f;
    String r_empty_str = "not empty";
    bool r_false_bool = true;

    reader << r_zero_int << r_zero_float << r_empty_str << r_false_bool;

    EXPECT_EQ(r_zero_int, 0);
    EXPECT_FLOAT_EQ(r_zero_float, 0.0f);
    EXPECT_EQ(r_empty_str, "");
    EXPECT_FALSE(r_false_bool);
}

// Negative Numbers
TEST_F(SerializationTest, NegativeNumbers)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    i8 neg_i8 = -100;
    i16 neg_i16 = -30000;
    i32 neg_i32 = -2000000000;
    i64 neg_i64 = -9000000000000000000LL;
    f32 neg_float = -123.456f;
    f64 neg_double = -987.654321;

    writer << neg_i8 << neg_i16 << neg_i32 << neg_i64 << neg_float << neg_double;

    MemoryReader reader(buffer);
    i8 r_neg_i8 = 0;
    i16 r_neg_i16 = 0;
    i32 r_neg_i32 = 0;
    i64 r_neg_i64 = 0;
    f32 r_neg_float = 0.0f;
    f64 r_neg_double = 0.0;

    reader << r_neg_i8 << r_neg_i16 << r_neg_i32 << r_neg_i64 << r_neg_float << r_neg_double;

    EXPECT_EQ(r_neg_i8, neg_i8);
    EXPECT_EQ(r_neg_i16, neg_i16);
    EXPECT_EQ(r_neg_i32, neg_i32);
    EXPECT_EQ(r_neg_i64, neg_i64);
    EXPECT_FLOAT_EQ(r_neg_float, neg_float);
    EXPECT_DOUBLE_EQ(r_neg_double, neg_double);
}

// Very Long String
TEST_F(SerializationTest, VeryLongString)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    String long_str;
    for (int i = 0; i < 1000; ++i)
    {
        long_str += "0123456789";
    }

    writer << long_str;

    MemoryReader reader(buffer);
    String read_str;
    reader << read_str;

    EXPECT_EQ(read_str, long_str);
    EXPECT_EQ(read_str.ByteLen(), 10000);
}

// Unicode Strings
TEST_F(SerializationTest, UnicodeStrings)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    String unicode1 = "한글 테스트";
    String unicode2 = "日本語テスト";
    String unicode3 = "中文测试";
    String emoji = "🚀🎉💻";

    writer << unicode1 << unicode2 << unicode3 << emoji;

    MemoryReader reader(buffer);
    String r_unicode1, r_unicode2, r_unicode3, r_emoji;
    reader << r_unicode1 << r_unicode2 << r_unicode3 << r_emoji;

    EXPECT_EQ(r_unicode1, unicode1);
    EXPECT_EQ(r_unicode2, unicode2);
    EXPECT_EQ(r_unicode3, unicode3);
    EXPECT_EQ(r_emoji, emoji);
}

// Single Element Containers
TEST_F(SerializationTest, SingleElementContainers)
{
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    Array<i32> single_array{42};
    HashMap<String, i32> single_map{ {"only", 123} };
    HashSet<String> single_set{ "unique" };

    writer << single_array << single_map << single_set;

    MemoryReader reader(buffer);
    Array<i32> r_single_array;
    HashMap<String, i32> r_single_map;
    HashSet<String> r_single_set;

    reader << r_single_array << r_single_map << r_single_set;

    EXPECT_EQ(r_single_array.Len(), 1);
    EXPECT_EQ(r_single_array[0], 42);
    EXPECT_EQ(r_single_map.Len(), 1);
    EXPECT_EQ(r_single_map["only"], 123);
    EXPECT_EQ(r_single_set.Len(), 1);
    EXPECT_TRUE(r_single_set.Contains("unique"));
}
