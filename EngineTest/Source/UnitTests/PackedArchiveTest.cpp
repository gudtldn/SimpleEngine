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
    writer.Int(42, EIntWidth::Bits32, true);

    PackedReader reader(buffer);
    i64 result = 0;
    reader.Int(result, EIntWidth::Bits32, true);

    EXPECT_EQ(result, 42);
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
        writer.Int(c.packed, c.width, c.is_signed);
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
    writer.Float(narrow_value, EFloatWidth::Bits32);
    writer.Float(wide_value, EFloatWidth::Bits64);

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
    writer.Bool(true);
    writer.Bool(false);

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
    writer.Present(true);
    writer.Present(false);

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
    writer.Enum(1, EIntWidth::Bits32, true, ArrayView<const EnumEntry>(entries));

    PackedReader reader(buffer);
    i64 result = 0;
    reader.Enum(result, EIntWidth::Bits32, true, ArrayView<const EnumEntry>(different_entries));

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, EnumUnsignedHighBitRoundTrip)
{
    // u32 enum의 0x80000000을 부호 있는 4바이트로 읽으면 음수가 되어 entries 대조가 실패합니다.
    // is_signed=false로 왕복하면 그대로 2147483648이어야 합니다.
    const i64 original = static_cast<i64>(u32{ 0x80000000 });

    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.Enum(original, EIntWidth::Bits32, false, {});

    PackedReader reader(buffer);
    i64 result = 0;
    reader.Enum(result, EIntWidth::Bits32, false, {});

    EXPECT_EQ(result, 2147483648);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, BytesRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    const u8 original[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    writer.Bytes(original, sizeof(original));

    PackedReader reader(buffer);
    u8 result[4] = {};
    reader.Bytes(result, sizeof(result));

    EXPECT_EQ(std::memcmp(original, result, sizeof(original)), 0);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, HugeBytesSizeSetsErrorWithoutOverflow)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.Bool(true); // offset > 0인 상태를 만듭니다.

    PackedReader reader(buffer);
    bool dummy = false;
    reader.Bool(dummy);

    u8 out[8] = {};
    reader.Bytes(out, std::numeric_limits<u64>::max()); // offset + size가 오버플로되는 크기

    EXPECT_TRUE(reader.HasError());
}

TEST(PackedArchiveTest, SeqCountRoundTrip)
{
    // Reader는 count가 남은 바이트 수를 넘으면 손상으로 보므로, 원소 수만큼 값을 함께 씁니다.
    constexpr usize count = 12345;
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginSeq(count, ESeqOrder::Ordered);
    for (usize index = 0; index < count; ++index)
    {
        writer.Bool(index % 2 == 0);
    }
    writer.EndSeq();

    PackedReader reader(buffer);
    u64 result_count = 0;
    reader.BeginSeq(result_count);

    EXPECT_EQ(result_count, count);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, MapCountRoundTrip)
{
    // Reader는 count가 남은 바이트 수를 넘으면 손상으로 보므로, 엔트리 수만큼 key와 value를 함께 씁니다.
    constexpr usize count = 777;
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginMap(count);
    for (usize index = 0; index < count; ++index)
    {
        writer.BeginMapEntry();
        writer.Int(static_cast<i64>(index), EIntWidth::Bits16, false);
        writer.Bool(true);
        writer.EndMapEntry();
    }
    writer.EndMap();

    PackedReader reader(buffer);
    u64 result_count = 0;
    reader.BeginMap(result_count);

    EXPECT_EQ(result_count, count);
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, OrderedAndUnorderedProduceSameBytes)
{
    Array<u8> ordered_buffer;
    PackedWriter ordered_writer(ordered_buffer);
    ordered_writer.BeginSeq(3, ESeqOrder::Ordered);

    Array<u8> unordered_buffer;
    PackedWriter unordered_writer(unordered_buffer);
    unordered_writer.BeginSeq(3, ESeqOrder::Unordered);

    ASSERT_EQ(ordered_buffer.Len(), unordered_buffer.Len());
    EXPECT_EQ(std::memcmp(ordered_buffer.Data(), unordered_buffer.Data(), ordered_buffer.Len()), 0);
}

TEST(PackedArchiveTest, SeqCountExceedingRemainingBytesSetsError)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginSeq(1000, ESeqOrder::Ordered); // count 4바이트만 있고, 뒤따르는 원소 데이터는 없습니다.

    PackedReader reader(buffer);
    u64 result_count = 999; // 센티넬
    reader.BeginSeq(result_count);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(result_count, 999u);
}

TEST(PackedArchiveTest, MapCountExceedingRemainingBytesSetsError)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginMap(1000);

    PackedReader reader(buffer);
    u64 result_count = 999;
    reader.BeginMap(result_count);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(result_count, 999u);
}

TEST(PackedArchiveTest, FieldWritesNoBytes)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.Int(7, EIntWidth::Bits32, true);

    writer.Field("SomeField");

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

TEST(PackedArchiveTest, SetErrorTwiceKeepsFirstMessage)
{
    Array<u8> empty_buffer;
    PackedReader reader(empty_buffer);

    i64 first = 0;
    reader.Int(first, EIntWidth::Bits32, true); // 첫 번째 에러
    ASSERT_TRUE(reader.HasError());
    const String first_message(reader.GetError());

    i64 second = 0;
    reader.Int(second, EIntWidth::Bits64, false); // 다른 조건으로 또 실패를 시도해도 무시되어야 합니다.
    EXPECT_EQ(reader.GetError(), first_message);
}

TEST(PackedArchiveTest, StrRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.Str("Hello, SimpleEngine!");
    writer.Str(""); // 빈 문자열도 왕복해야 합니다.

    PackedReader reader(buffer);
    String first;
    String second;
    reader.Str(first);
    reader.Str(second);

    EXPECT_EQ(first, "Hello, SimpleEngine!");
    EXPECT_EQ(second, "");
    EXPECT_FALSE(reader.HasError());
}

TEST(PackedArchiveTest, StrLengthExceedingRemainingBytesSetsErrorWithoutGrowingOutString)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.Str("this string is definitely longer than zero bytes");

    // 길이 접두(4바이트)만 남기고 실제 문자 데이터는 잘라내, 손상된 스트림을 흉내냅니다.
    buffer.Truncate(sizeof(u32));

    PackedReader reader(buffer);
    String value = "sentinel";
    reader.Str(value);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(value, "sentinel"); // 실패했으므로 out 문자열이 커지거나 바뀌면 안 됩니다.
}
