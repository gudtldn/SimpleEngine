#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Container/Set.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"

using namespace se;
using namespace se::core;

class SerializationTest : public ::testing::Test
{
};

TEST_F(SerializationTest, ReadAndWritePrimitives)
{
    Array<uint8> buffer;
    MemoryWriter writer(buffer);

    // 원본 데이터
    int32 original_int = -12345;
    float original_float = 3.14159f;
    bool original_bool = true;
    double original_double = 1.23456789;

    // 쓰기
    writer << original_int << original_float << original_bool << original_double;

    constexpr usize expected_size = sizeof(original_int) + sizeof(original_float) + sizeof(original_bool) + sizeof(original_double);
    EXPECT_EQ(buffer.Len(), expected_size);
    EXPECT_EQ(writer.Tell(), expected_size);

    // 읽기
    MemoryReader reader(buffer);
    int32 read_int = 0;
    float read_float = 0.0f;
    bool read_bool = false;
    double read_double = 0.0;

    reader << read_int << read_float << read_bool << read_double;

    // 검증
    EXPECT_EQ(read_int, original_int);
    EXPECT_FLOAT_EQ(read_float, original_float);
    EXPECT_EQ(read_bool, original_bool);
    EXPECT_DOUBLE_EQ(read_double, original_double);
    EXPECT_EQ(reader.Tell(), expected_size);
}

TEST_F(SerializationTest, SeekAndTell)
{
    Array<uint8> buffer;
    MemoryWriter writer(buffer);

    int32 val1 = 100;
    int32 val2 = 200;
    int32 val3 = 300; // 덮어쓸 값

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
    int32 read_val1 = 0;
    int32 read_val2 = 0;

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
    float b;
    bool c;

    auto operator<=>(const TestStruct&) const = default;
};

void Serialize(Archive& ar, TestStruct& value)
{
    ar("a") << value.a;
    ar("b") << value.b;
    ar("c") << value.c;
}
}

TEST_F(SerializationTest, SerializeStruct)
{
    Array<uint8> buffer;
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
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        Array<int32> original_array = { 1, 2, 3, 4, 5 };
        Array<int32> read_array;
        writer << original_array;
        MemoryReader reader(buffer);
        reader << read_array;
        EXPECT_EQ(read_array, original_array);
    }

    // 2. Non-trivial type (String)
    {
        Array<uint8> buffer;
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
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        Array<int32> original_array;
        Array<int32> read_array = { 1, 2, 3 }; // Not empty initially
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
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        HashMap<int32, float> original_map = { { 1, 1.1f }, { 2, 2.2f }, { 3, 3.3f } };
        HashMap<int32, float> read_map;
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_EQ(read_map, original_map);
    }

    // 2. Non-trivial Value
    {
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        HashMap<int32, String> original_map = { { 1, "One" }, { 2, "Two" }, { 3, "Three" } };
        HashMap<int32, String> read_map;
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_EQ(read_map, original_map);
    }

    // 3. Empty map
    {
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        HashMap<int32, int32> original_map;
        HashMap<int32, int32> read_map = { { 1, 1 } };
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
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        HashSet<int32> original_set = { 1, 2, 3, 100, 200 };
        HashSet<int32> read_set;
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_EQ(read_set, original_set);
    }

    // 2. Non-trivial Key
    {
        Array<uint8> buffer;
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
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        HashSet<int32> original_set;
        HashSet<int32> read_set = { 1, 2, 3 };
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
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        Map<int32, float> original_map = { { 1, 1.1f }, { 3, 3.3f }, { 2, 2.2f } };
        Map<int32, float> read_map;
        writer << original_map;
        MemoryReader reader(buffer);
        reader << read_map;
        EXPECT_EQ(read_map, original_map);
    }

    // 2. Non-trivial Value
    {
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        Map<String, int32> original_map = { { "One", 1 }, { "Two", 2 }, { "Three", 3 } };
        Map<String, int32> read_map;
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
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        Set<int32> original_set = { 1, 100, 3, 200, 2 };
        Set<int32> read_set;
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_EQ(read_set, original_set);
    }

    // 2. Non-trivial Key
    {
        Array<uint8> buffer;
        MemoryWriter writer(buffer);
        Set<String> original_set = { "Hello", "A", "World", "C" };
        Set<String> read_set;
        writer << original_set;
        MemoryReader reader(buffer);
        reader << read_set;
        EXPECT_EQ(read_set, original_set);
    }
}