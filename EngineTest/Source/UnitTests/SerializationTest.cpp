#include "gtest/gtest.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"
#include "SimpleEngine/Core/Container/Array.h"

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
    int32_t original_int = -12345;
    float original_float = 3.14159f;
    bool original_bool = true;
    double original_double = 1.23456789;

    // 쓰기
    writer << original_int
           << original_float
           << original_bool
           << original_double;

    constexpr usize expected_size = sizeof(original_int) + sizeof(original_float) + sizeof(original_bool) + sizeof(original_double);
    EXPECT_EQ(buffer.Len(), expected_size);
    EXPECT_EQ(writer.Tell(), expected_size);

    // 읽기
    MemoryReader reader(buffer);
    int32_t read_int = 0;
    float read_float = 0.0f;
    bool read_bool = false;
    double read_double = 0.0;

    reader << read_int
           << read_float
           << read_bool
           << read_double;

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

    int32_t val1 = 100;
    int32_t val2 = 200;
    int32_t val3 = 300; // 덮어쓸 값

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
    int32_t read_val1 = 0;
    int32_t read_val2 = 0;

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

struct TestStruct
{
    int a;
    float b;
    bool c;

    bool operator==(const TestStruct& other) const
    {
        return a == other.a && b == other.b && c == other.c;
    }
};

void Serialize(Archive& ar, TestStruct& value)
{
    ar << BinaryData::FromItems(&value);
}

TEST_F(SerializationTest, SerializeStruct)
{
    Array<uint8> buffer;
    MemoryWriter writer(buffer);

    TestStruct original_struct = { -1, 123.456f, true };
    TestStruct read_struct = { 0, 0.0f, false };

    // 쓰기
    writer << original_struct;

    EXPECT_EQ(writer.Tell(), sizeof(TestStruct));
    EXPECT_EQ(buffer.Len(), sizeof(TestStruct));

    // 읽기
    MemoryReader reader(buffer);
    reader << read_struct;

    // 검증
    EXPECT_EQ(read_struct, original_struct);
    EXPECT_EQ(reader.Tell(), sizeof(TestStruct));
}
