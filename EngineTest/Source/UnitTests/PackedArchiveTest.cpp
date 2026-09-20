#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"

#include <cstring>
#include <limits>

using namespace se;

TEST(PackedArchiveTest, ScalarRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    i64 original = 42;
    writer.Int(original, EIntWidth::Bits32, true);

    PackedReader reader(buffer);
    i64 result = 0;
    reader.Int(result, EIntWidth::Bits32, true);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, AllIntWidthsRoundTrip)
{
    struct Case
    {
        i64 packed;
        EIntWidth width;
        bool is_signed;
    };

    // u64 최상위 비트는 static_cast<i64>의 modular 변환(C++20~)으로 비트 패턴이 그대로 보존됩니다.
    const Case cases[] = {
        { static_cast<i64>(std::numeric_limits<i8>::min()),  EIntWidth::Bits8,  true },
        { static_cast<i64>(std::numeric_limits<i8>::max()),  EIntWidth::Bits8,  true },
        { static_cast<i64>(std::numeric_limits<u8>::max()),  EIntWidth::Bits8,  false },
        { static_cast<i64>(std::numeric_limits<i16>::min()), EIntWidth::Bits16, true },
        { static_cast<i64>(std::numeric_limits<i16>::max()), EIntWidth::Bits16, true },
        { static_cast<i64>(std::numeric_limits<u16>::max()), EIntWidth::Bits16, false },
        { static_cast<i64>(std::numeric_limits<i32>::min()), EIntWidth::Bits32, true },
        { static_cast<i64>(std::numeric_limits<i32>::max()), EIntWidth::Bits32, true },
        { static_cast<i64>(std::numeric_limits<u32>::max()), EIntWidth::Bits32, false },
        { std::numeric_limits<i64>::min(),                   EIntWidth::Bits64, true },
        { std::numeric_limits<i64>::max(),                   EIntWidth::Bits64, true },
        { static_cast<i64>(std::numeric_limits<u64>::max()), EIntWidth::Bits64, false },
    };

    Array<u8> buffer;
    PackedWriter writer(buffer);
    for (const Case& c : cases)
    {
        i64 value = c.packed;
        writer.Int(value, c.width, c.is_signed);
    }

    PackedReader reader(buffer);
    for (const Case& c : cases)
    {
        i64 result = 0;
        reader.Int(result, c.width, c.is_signed);
        EXPECT_EQ(result, c.packed);
    }
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, FloatWidthsRoundTrip)
{
    const f32 exact_f32 = 3.14159f;
    const f64 narrow_value = static_cast<f64>(exact_f32);
    const f64 wide_value = 1.23456789012345;

    Array<u8> buffer;
    PackedWriter writer(buffer);
    f64 write_narrow = narrow_value;
    f64 write_wide = wide_value;
    writer.Float(write_narrow, EFloatWidth::Bits32);
    writer.Float(write_wide, EFloatWidth::Bits64);

    PackedReader reader(buffer);
    f64 read_narrow = 0.0;
    f64 read_wide = 0.0;
    reader.Float(read_narrow, EFloatWidth::Bits32);
    reader.Float(read_wide, EFloatWidth::Bits64);

    // f32로 정확히 표현되는 값을 좁혔다 되읽었으므로 손실이 없어야 합니다.
    EXPECT_EQ(read_narrow, narrow_value);
    EXPECT_DOUBLE_EQ(read_wide, wide_value);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, BoolRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    bool original_true = true;
    bool original_false = false;
    writer.Bool(original_true);
    writer.Bool(original_false);

    PackedReader reader(buffer);
    bool read_true = false;
    bool read_false = true;
    reader.Bool(read_true);
    reader.Bool(read_false);

    EXPECT_TRUE(read_true);
    EXPECT_FALSE(read_false);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, PresentRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    bool has_value = true;
    bool no_value = false;
    writer.Present(has_value);
    writer.Present(no_value);

    PackedReader reader(buffer);
    bool read_has = false;
    bool read_no = true;
    reader.Present(read_has);
    reader.Present(read_no);

    EXPECT_TRUE(read_has);
    EXPECT_FALSE(read_no);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, EnumRoundTrip)
{
    const EnumEntry entries[] = {
        { .value = 0, .name = "Sword" },
        { .value = 1, .name = "Bow" },
    };
    // 쓸 때와 읽을 때 서로 다른 entries를 넘겨도 바이너리 결과는 같아야 합니다 (entries는 무시됨).
    const EnumEntry different_entries[] = {
        { .value = 99, .name = "Unrelated" },
    };

    Array<u8> buffer;
    PackedWriter writer(buffer);
    i64 value = 1;
    writer.Enum(value, ArrayView<const EnumEntry>(entries), EIntWidth::Bits32);

    PackedReader reader(buffer);
    i64 result = 0;
    reader.Enum(result, ArrayView<const EnumEntry>(different_entries), EIntWidth::Bits32);

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, BytesRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    u8 original[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    writer.Bytes(original, sizeof(original));

    PackedReader reader(buffer);
    u8 result[4] = {};
    reader.Bytes(result, sizeof(result));

    EXPECT_EQ(std::memcmp(original, result, sizeof(original)), 0);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, SeqCountRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    u64 original_count = 12345;
    writer.BeginSeq(original_count);

    PackedReader reader(buffer);
    u64 result_count = 0;
    reader.BeginSeq(result_count);

    EXPECT_EQ(result_count, original_count);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, FieldWritesNoBytes)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    i64 before = 7;
    writer.Int(before, EIntWidth::Bits32, true);

    const bool field_result = writer.Field(1, "SomeField", 0);
    EXPECT_TRUE(field_result);

    // Field가 바이트를 쓰지 않았다면 버퍼 길이는 Int 하나 분량(4바이트) 그대로여야 합니다.
    EXPECT_EQ(buffer.Len(), sizeof(i32));
}

TEST(PackedArchiveTest, ReadPastEndSetsError)
{
    Array<u8> empty_buffer;
    PackedReader reader(empty_buffer);

    i64 value = 999; // 센티넬 - 실패 시 바뀌면 안 됩니다.
    reader.Int(value, EIntWidth::Bits32, true);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(value, 999);
}

TEST(PackedArchiveTest, ErrorIsSticky)
{
    Array<u8> small_buffer;
    small_buffer.Push(u8{ 0 }); // 1바이트만 존재

    PackedReader reader(small_buffer);

    i64 first = 0;
    reader.Int(first, EIntWidth::Bits32, true); // 4바이트 요구 -> 실패, 에러 설정
    ASSERT_TRUE(reader.HasError());

    i64 second = 555; // 센티넬
    reader.Int(second, EIntWidth::Bits8, true); // 버퍼에 1바이트가 남아있어도 no-op이어야 함
    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(second, 555);
}
