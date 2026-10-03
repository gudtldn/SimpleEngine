#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"

#include <cstring>
#include <limits>

using namespace se;

TEST(BinaryArchiveTest, ScalarRoundTrip)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Int(42, EIntWidth::Bits32, true);

    BinaryReader reader(buffer);
    i64 result = 0;
    reader.Int(result, EIntWidth::Bits32, true);

    EXPECT_EQ(result, 42);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, AllIntWidthsRoundTrip)
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
    BinaryWriter writer(buffer);
    for (const Case& c : cases)
    {
        writer.Int(c.packed, c.width, c.is_signed);
    }

    BinaryReader reader(buffer);
    for (const Case& c : cases)
    {
        i64 result = 0;
        reader.Int(result, c.width, c.is_signed);
        EXPECT_EQ(result, c.packed);
    }
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, FloatWidthsRoundTrip)
{
    const f32 exact_f32 = 3.14159f;
    const f64 narrow_value = static_cast<f64>(exact_f32);
    const f64 wide_value = 1.23456789012345;

    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Float(narrow_value, EFloatWidth::Bits32);
    writer.Float(wide_value, EFloatWidth::Bits64);

    BinaryReader reader(buffer);
    f64 read_narrow = 0.0;
    f64 read_wide = 0.0;
    reader.Float(read_narrow, EFloatWidth::Bits32);
    reader.Float(read_wide, EFloatWidth::Bits64);

    // f32로 정확히 표현되는 값을 좁혔다 되읽었으므로 손실이 없어야 합니다.
    EXPECT_EQ(read_narrow, narrow_value);
    EXPECT_DOUBLE_EQ(read_wide, wide_value);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, BoolRoundTrip)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Bool(true);
    writer.Bool(false);

    BinaryReader reader(buffer);
    bool read_true = false;
    bool read_false = true;
    reader.Bool(read_true);
    reader.Bool(read_false);

    EXPECT_TRUE(read_true);
    EXPECT_FALSE(read_false);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, PresentRoundTrip)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Present(true);
    writer.Present(false);

    BinaryReader reader(buffer);
    bool read_has = false;
    bool read_no = true;
    reader.Present(read_has);
    reader.Present(read_no);

    EXPECT_TRUE(read_has);
    EXPECT_FALSE(read_no);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, EnumRoundTrip)
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
    BinaryWriter writer(buffer);
    writer.Enum(1, EIntWidth::Bits32, true, ArrayView<const EnumEntry>(entries));

    BinaryReader reader(buffer);
    i64 result = 0;
    reader.Enum(result, EIntWidth::Bits32, true, ArrayView<const EnumEntry>(different_entries));

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, EnumUnsignedHighBitRoundTrip)
{
    // u32 enum의 0x80000000을 부호 있는 4바이트로 읽으면 음수가 되어 entries 대조가 실패합니다.
    // is_signed=false로 왕복하면 그대로 2147483648이어야 합니다.
    const i64 original = static_cast<i64>(u32{ 0x80000000 });

    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Enum(original, EIntWidth::Bits32, false, {});

    BinaryReader reader(buffer);
    i64 result = 0;
    reader.Enum(result, EIntWidth::Bits32, false, {});

    EXPECT_EQ(result, 2147483648);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, BytesRoundTrip)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    const u8 original[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    writer.Bytes(original, sizeof(original));

    BinaryReader reader(buffer);
    u8 result[4] = {};
    reader.Bytes(result, sizeof(result));

    EXPECT_EQ(std::memcmp(original, result, sizeof(original)), 0);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, ZeroSizeBytesWithNullPointerRoundTrip)
{
    // 빈 컨테이너의 데이터 포인터처럼 nullptr와 크기 0을 넘겨도 아무것도 쓰거나 읽지 않아야 합니다.
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Bytes(nullptr, 0);
    writer.RawElements(nullptr, 0);
    writer.Bool(true);

    EXPECT_EQ(buffer.Len(), 1);

    BinaryReader reader(buffer);
    reader.Bytes(nullptr, 0);
    reader.RawElements(nullptr, 0);
    bool value = false;
    reader.Bool(value);

    EXPECT_TRUE(value);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, HugeBytesSizeSetsErrorWithoutOverflow)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Bool(true); // offset > 0인 상태를 만듭니다.

    BinaryReader reader(buffer);
    bool dummy = false;
    reader.Bool(dummy);

    u8 out[8] = {};
    reader.Bytes(out, std::numeric_limits<u64>::max()); // offset + size가 오버플로되는 크기

    EXPECT_TRUE(reader.HasError());
}

TEST(BinaryArchiveTest, SeqCountRoundTrip)
{
    // Reader는 count가 남은 바이트 수를 넘으면 손상으로 보므로, 원소 수만큼 값을 함께 씁니다.
    constexpr usize count = 12345;
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.BeginSeq(count, ESeqOrder::Ordered);
    for (usize index = 0; index < count; ++index)
    {
        writer.Bool(index % 2 == 0);
    }
    writer.EndSeq();

    BinaryReader reader(buffer);
    u64 result_count = 0;
    reader.BeginSeq(result_count);

    EXPECT_EQ(result_count, count);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, MapCountRoundTrip)
{
    // Reader는 count가 남은 바이트 수를 넘으면 손상으로 보므로, 엔트리 수만큼 key와 value를 함께 씁니다.
    constexpr usize count = 777;
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.BeginMap(count);
    for (usize index = 0; index < count; ++index)
    {
        writer.BeginMapEntry();
        writer.Int(static_cast<i64>(index), EIntWidth::Bits16, false);
        writer.Bool(true);
        writer.EndMapEntry();
    }
    writer.EndMap();

    BinaryReader reader(buffer);
    u64 result_count = 0;
    reader.BeginMap(result_count);

    EXPECT_EQ(result_count, count);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, OrderedAndUnorderedProduceSameBytes)
{
    Array<u8> ordered_buffer;
    BinaryWriter ordered_writer(ordered_buffer);
    ordered_writer.BeginSeq(3, ESeqOrder::Ordered);

    Array<u8> unordered_buffer;
    BinaryWriter unordered_writer(unordered_buffer);
    unordered_writer.BeginSeq(3, ESeqOrder::Unordered);

    ASSERT_EQ(ordered_buffer.Len(), unordered_buffer.Len());
    EXPECT_EQ(std::memcmp(ordered_buffer.Data(), unordered_buffer.Data(), ordered_buffer.Len()), 0);
}

TEST(BinaryArchiveTest, SeqCountExceedingRemainingBytesSetsError)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.BeginSeq(1000, ESeqOrder::Ordered); // count 4바이트만 있고, 뒤따르는 원소 데이터는 없습니다.

    BinaryReader reader(buffer);
    u64 result_count = 999; // 센티넬
    reader.BeginSeq(result_count);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(result_count, 999u);
}

TEST(BinaryArchiveTest, MapCountExceedingRemainingBytesSetsError)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.BeginMap(1000);

    BinaryReader reader(buffer);
    u64 result_count = 999;
    reader.BeginMap(result_count);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(result_count, 999u);
}

TEST(BinaryArchiveTest, FieldWritesNoBytes)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Int(7, EIntWidth::Bits32, true);

    writer.Field("SomeField");

    // Field가 바이트를 쓰지 않았다면 버퍼 길이는 Int 하나 분량(4바이트) 그대로여야 합니다.
    EXPECT_EQ(buffer.Len(), sizeof(i32));
}

TEST(BinaryArchiveTest, ReadPastEndSetsError)
{
    Array<u8> empty_buffer;
    BinaryReader reader(empty_buffer);

    i64 value = 999; // 센티넬 - 실패 시 바뀌면 안 됩니다.
    reader.Int(value, EIntWidth::Bits32, true);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(value, 999);
}

TEST(BinaryArchiveTest, ErrorIsSticky)
{
    Array<u8> small_buffer;
    small_buffer.Push(u8{ 0 }); // 1바이트만 존재

    BinaryReader reader(small_buffer);

    i64 first = 0;
    reader.Int(first, EIntWidth::Bits32, true); // 4바이트 요구 -> 실패, 에러 설정
    ASSERT_TRUE(reader.HasError());

    i64 second = 555; // 센티넬
    reader.Int(second, EIntWidth::Bits8, true); // 버퍼에 1바이트가 남아있어도 no-op이어야 함
    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(second, 555);
}

TEST(BinaryArchiveTest, SetErrorTwiceKeepsFirstMessage)
{
    Array<u8> empty_buffer;
    BinaryReader reader(empty_buffer);

    i64 first = 0;
    reader.Int(first, EIntWidth::Bits32, true); // 첫 번째 에러
    ASSERT_TRUE(reader.HasError());
    const String first_message(reader.GetError());

    i64 second = 0;
    reader.Int(second, EIntWidth::Bits64, false); // 다른 조건으로 또 실패를 시도해도 무시되어야 합니다.
    EXPECT_EQ(reader.GetError(), first_message);
}

TEST(BinaryArchiveTest, StrRoundTrip)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Str("Hello, SimpleEngine!");
    writer.Str(""); // 빈 문자열도 왕복해야 합니다.

    BinaryReader reader(buffer);
    String first;
    String second;
    reader.Str(first);
    reader.Str(second);

    EXPECT_EQ(first, "Hello, SimpleEngine!");
    EXPECT_EQ(second, "");
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, StrLengthExceedingRemainingBytesSetsErrorWithoutGrowingOutString)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Str("this string is definitely longer than zero bytes");

    // 길이 접두(4바이트)만 남기고 실제 문자 데이터는 잘라내, 손상된 스트림을 흉내냅니다.
    buffer.Truncate(sizeof(u32));

    BinaryReader reader(buffer);
    String value = "sentinel";
    reader.Str(value);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(value, "sentinel"); // 실패했으므로 out 문자열이 커지거나 바뀌면 안 됩니다.
}


// --- 구간과 Rewind ---

TEST(BinaryArchiveTest, SectionRoundTrip)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Bool(true);
    writer.BeginSection();
    writer.BeginStruct();
    writer.Field("id");
    writer.Int(7, EIntWidth::Bits32, true);
    writer.Field("name");
    writer.Str("inside");
    writer.EndStruct();
    writer.EndSection();
    writer.Int(42, EIntWidth::Bits16, false);
    ASSERT_FALSE(writer.HasError());

    BinaryReader reader(buffer);
    bool flag = false;
    i64 id = 0;
    String name;
    i64 tail = 0;
    reader.Bool(flag);
    reader.BeginSection();
    reader.BeginStruct();
    ASSERT_TRUE(reader.Field("id"));
    reader.Int(id, EIntWidth::Bits32, true);
    ASSERT_TRUE(reader.Field("name"));
    reader.Str(name);
    reader.EndStruct();
    reader.EndSection();
    reader.Int(tail, EIntWidth::Bits16, false);

    EXPECT_TRUE(flag);
    EXPECT_EQ(id, 7);
    EXPECT_EQ(name, "inside");
    EXPECT_EQ(tail, 42);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, SectionWritesLengthPrefix)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.BeginSection();
    writer.BeginStruct();
    writer.Field("a");
    writer.Int(7, EIntWidth::Bits32, true);
    writer.Field("b");
    writer.Str("abc");
    writer.EndStruct();
    writer.EndSection();
    ASSERT_FALSE(writer.HasError());

    // 길이(8) | i32(4) | 문자열 길이(4) | "abc"(3)
    ASSERT_EQ(buffer.Len(), 8u + 4u + 4u + 3u);
    u64 length = 0;
    std::memcpy(&length, buffer.Data(), sizeof(length));
    EXPECT_EQ(length, 11u);
}

TEST(BinaryArchiveTest, NestedSectionsRoundTrip)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.BeginSection();
    writer.BeginStruct();
    writer.Field("inner");
    writer.BeginSection();
    writer.Int(5, EIntWidth::Bits32, true);
    writer.EndSection();
    writer.Field("after");
    writer.Int(6, EIntWidth::Bits32, true);
    writer.EndStruct();
    writer.EndSection();
    ASSERT_FALSE(writer.HasError());

    BinaryReader reader(buffer);
    i64 inner = 0;
    i64 after = 0;
    reader.BeginSection();
    reader.BeginStruct();
    ASSERT_TRUE(reader.Field("inner"));
    reader.BeginSection();
    reader.Int(inner, EIntWidth::Bits32, true);
    reader.EndSection();
    ASSERT_TRUE(reader.Field("after"));
    reader.Int(after, EIntWidth::Bits32, true);
    reader.EndStruct();
    reader.EndSection();

    EXPECT_EQ(inner, 5);
    EXPECT_EQ(after, 6);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, SkipSectionJumpsToNextValue)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.BeginSection();
    writer.BeginStruct();
    writer.Field("inner");
    writer.BeginSection();
    writer.Int(5, EIntWidth::Bits32, true);
    writer.EndSection();
    writer.Field("after");
    writer.Int(6, EIntWidth::Bits32, true);
    writer.EndStruct();
    writer.EndSection();
    writer.Int(42, EIntWidth::Bits32, true);
    ASSERT_FALSE(writer.HasError());

    BinaryReader reader(buffer);
    reader.SkipSection();
    i64 tail = 0;
    reader.Int(tail, EIntWidth::Bits32, true);

    EXPECT_EQ(tail, 42);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, SectionReadShortOrLongIsError)
{
    {
        // 덜 읽음: b를 읽지 않고 구간을 닫음
        Array<u8> buffer;
        BinaryWriter writer(buffer);
        writer.BeginSection();
        writer.BeginStruct();
        writer.Field("a");
        writer.Int(1, EIntWidth::Bits32, true);
        writer.Field("b");
        writer.Int(2, EIntWidth::Bits32, true);
        writer.EndStruct();
        writer.EndSection();
        ASSERT_FALSE(writer.HasError());

        BinaryReader reader(buffer);
        i64 a = 0;
        reader.BeginSection();
        reader.BeginStruct();
        ASSERT_TRUE(reader.Field("a"));
        reader.Int(a, EIntWidth::Bits32, true);
        reader.EndStruct();
        reader.EndSection();

        ASSERT_TRUE(reader.HasError());
        EXPECT_TRUE(String(reader.GetError()).Contains("section ends at offset"));
    }
    {
        // 더 읽음: 구간 뒤의 값까지 읽고 구간을 닫음
        Array<u8> buffer;
        BinaryWriter writer(buffer);
        writer.BeginSection();
        writer.Int(1, EIntWidth::Bits32, true);
        writer.EndSection();
        writer.Int(2, EIntWidth::Bits32, true);
        ASSERT_FALSE(writer.HasError());

        BinaryReader reader(buffer);
        i64 first = 0;
        i64 second = 0;
        reader.BeginSection();
        reader.Int(first, EIntWidth::Bits32, true);
        reader.Int(second, EIntWidth::Bits32, true);
        reader.EndSection();

        ASSERT_TRUE(reader.HasError());
        EXPECT_TRUE(String(reader.GetError()).Contains("section ends at offset"));
    }
}

TEST(BinaryArchiveTest, TruncatedSectionIsError)
{
    const auto write_section = [](Array<u8>& out_buffer)
    {
        BinaryWriter writer(out_buffer);
        writer.BeginSection();
        writer.Str("payload");
        writer.EndSection();
    };

    // 마지막 1바이트를 잘라 길이가 남은 바이트 수를 넘게 함
    Array<u8> cut_tail;
    write_section(cut_tail);
    cut_tail.Truncate(cut_tail.Len() - 1);

    BinaryReader begin_reader(cut_tail);
    begin_reader.BeginSection();
    ASSERT_TRUE(begin_reader.HasError());
    EXPECT_TRUE(String(begin_reader.GetError()).Contains("exceeds remaining bytes"));

    BinaryReader skip_reader(cut_tail);
    skip_reader.SkipSection();
    ASSERT_TRUE(skip_reader.HasError());
    EXPECT_TRUE(String(skip_reader.GetError()).Contains("exceeds remaining bytes"));

    // 길이 필드(8바이트) 자체가 잘린 경우
    Array<u8> cut_length;
    write_section(cut_length);
    cut_length.Truncate(4);

    BinaryReader short_reader(cut_length);
    short_reader.BeginSection();
    EXPECT_TRUE(short_reader.HasError());
}

TEST(BinaryArchiveTest, UnmatchedEndSectionIsError)
{
    Array<u8> write_buffer;
    BinaryWriter writer(write_buffer);
    writer.EndSection();
    EXPECT_EQ(String(writer.GetError()), "BinaryWriter: EndSection does not match an open section.");

    Array<u8> read_buffer;
    BinaryReader reader(read_buffer);
    reader.EndSection();
    EXPECT_EQ(String(reader.GetError()), "BinaryReader: EndSection does not match an open section.");
}

TEST(BinaryArchiveTest, RewindReadsSameValuesAgain)
{
    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.Int(7, EIntWidth::Bits32, true);
    writer.BeginSection();
    writer.Str("abc");
    writer.EndSection();
    ASSERT_FALSE(writer.HasError());

    BinaryReader reader(buffer);
    const auto read_all = [&reader]()
    {
        i64 number = 0;
        String text;
        reader.Int(number, EIntWidth::Bits32, true);
        reader.BeginSection();
        reader.Str(text);
        reader.EndSection();
        EXPECT_EQ(number, 7);
        EXPECT_EQ(text, "abc");
    };

    read_all();
    reader.Rewind();
    read_all();
    ASSERT_FALSE(reader.HasError());

    // 구간 중간의 Rewind는 열린 구간을 비움
    reader.Rewind();
    i64 number = 0;
    reader.Int(number, EIntWidth::Bits32, true);
    reader.BeginSection();
    reader.Rewind();
    read_all();
    EXPECT_FALSE(reader.HasError());

    reader.EndSection();
    ASSERT_TRUE(reader.HasError());
    EXPECT_TRUE(String(reader.GetError()).Contains("does not match an open section"));
}

TEST(BinaryArchiveTest, RewindKeepsError)
{
    Array<u8> empty_buffer;
    BinaryReader reader(empty_buffer);

    i64 value = 0;
    reader.Int(value, EIntWidth::Bits32, true);
    ASSERT_TRUE(reader.HasError());
    const String message(reader.GetError());

    reader.Rewind();

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(String(reader.GetError()), message);
}


// --- BinaryFileWriter / BinaryFileReader ---

namespace
{
constexpr u64 TEST_SCHEMA_HASH = 0x0123456789ABCDEFULL;

/** Int 노드 하나(i32 42)를 payload로 갖는 파일용 버퍼를 만듭니다. 루트 타입은 i32입니다. */
[[nodiscard]] Array<u8> MakeFileBuffer()
{
    Array<u8> buffer;
    BinaryFileWriter writer(buffer, TypeId::Of<i32>(), TEST_SCHEMA_HASH);
    writer.Int(42, EIntWidth::Bits32, true);
    writer.Finish();
    return buffer;
}

/** buffer를 BinaryFileReader로 열었을 때의 오류 메시지를 돌려줍니다. 헤더가 맞으면 빈 문자열입니다. */
[[nodiscard]] String OpenError(ArrayView<const u8> buffer, TypeId root_type = TypeId::Of<i32>(), u64 schema_hash = TEST_SCHEMA_HASH)
{
    const BinaryFileReader reader(buffer, root_type, schema_hash);
    return String(reader.GetError());
}
} // namespace

TEST(BinaryArchiveTest, FileRoundTripReadsPayload)
{
    const Array<u8> buffer = MakeFileBuffer();
    EXPECT_EQ(buffer.Len(), sizeof(BinaryFileHeader) + sizeof(i32));

    BinaryFileReader reader(buffer, TypeId::Of<i32>(), TEST_SCHEMA_HASH);
    i64 result = 0;
    reader.Int(result, EIntWidth::Bits32, true);

    EXPECT_FALSE(reader.HasError());
    EXPECT_EQ(result, 42);
}

TEST(BinaryArchiveTest, FileReaderRewindsToPayloadStart)
{
    const Array<u8> buffer = MakeFileBuffer();

    BinaryFileReader reader(buffer, TypeId::Of<i32>(), TEST_SCHEMA_HASH);
    i64 first = 0;
    reader.Int(first, EIntWidth::Bits32, true);

    // 헤더가 아니라 payload 시작으로 돌아가야 같은 값을 다시 읽음
    reader.Rewind();
    i64 second = 0;
    reader.Int(second, EIntWidth::Bits32, true);

    EXPECT_EQ(first, 42);
    EXPECT_EQ(second, 42);
    EXPECT_FALSE(reader.HasError());
}

TEST(BinaryArchiveTest, FileHeaderRecordsLayoutFields)
{
    const Array<u8> buffer = MakeFileBuffer();

    // magic(0) | wire 버전(4) | 루트 TypeId(8) | 스키마 해시(16) | payload 크기(24) | 체크섬(32)
    // BinaryFileHeader의 필드 순서가 바뀌어도 왕복은 통과하므로, 저장 배치는 바이트 위치로 고정합니다.
    EXPECT_EQ(std::memcmp(buffer.Data(), "SEBN", 4), 0);

    u32 wire_version = 0;
    std::memcpy(&wire_version, buffer.Data() + 4, sizeof(wire_version));
    EXPECT_EQ(wire_version, 1u);

    u64 root_type = 0;
    std::memcpy(&root_type, buffer.Data() + 8, sizeof(root_type));
    EXPECT_EQ(root_type, TypeId::Of<i32>().Value());

    u64 schema_hash = 0;
    std::memcpy(&schema_hash, buffer.Data() + 16, sizeof(schema_hash));
    EXPECT_EQ(schema_hash, TEST_SCHEMA_HASH);

    u64 payload_size = 0;
    std::memcpy(&payload_size, buffer.Data() + 24, sizeof(payload_size));
    EXPECT_EQ(payload_size, sizeof(i32));
}

TEST(BinaryArchiveTest, FileTooShortForHeaderSetsError)
{
    Array<u8> buffer = MakeFileBuffer();
    buffer.Truncate(sizeof(BinaryFileHeader) - 1);

    EXPECT_TRUE(OpenError(buffer).Contains("too short"));
}

TEST(BinaryArchiveTest, FileMagicMismatchSetsError)
{
    Array<u8> buffer = MakeFileBuffer();
    buffer[0] = 'X';

    EXPECT_TRUE(OpenError(buffer).Contains("magic"));
}

TEST(BinaryArchiveTest, FileWireVersionMismatchSetsError)
{
    Array<u8> buffer = MakeFileBuffer();
    buffer[4] = static_cast<u8>(buffer[4] + 1);

    EXPECT_TRUE(OpenError(buffer).Contains("wire version"));
}

TEST(BinaryArchiveTest, FileRootTypeMismatchSetsError)
{
    const Array<u8> buffer = MakeFileBuffer();

    EXPECT_TRUE(OpenError(buffer, TypeId::Of<f32>()).Contains("root type"));
}

TEST(BinaryArchiveTest, FileSchemaHashMismatchSetsError)
{
    const Array<u8> buffer = MakeFileBuffer();

    EXPECT_TRUE(OpenError(buffer, TypeId::Of<i32>(), TEST_SCHEMA_HASH + 1).Contains("schema hash"));
}

TEST(BinaryArchiveTest, FileWithTruncatedPayloadSetsError)
{
    Array<u8> buffer = MakeFileBuffer();
    buffer.Truncate(buffer.Len() - 1);

    EXPECT_TRUE(OpenError(buffer).Contains("payload size"));
}

TEST(BinaryArchiveTest, FileWithTrailingBytesSetsError)
{
    Array<u8> buffer = MakeFileBuffer();
    buffer.Push(0);

    EXPECT_TRUE(OpenError(buffer).Contains("payload size"));
}

TEST(BinaryArchiveTest, FileWithCorruptedPayloadSetsError)
{
    Array<u8> buffer = MakeFileBuffer();
    buffer[sizeof(BinaryFileHeader)] = static_cast<u8>(buffer[sizeof(BinaryFileHeader)] ^ 0xFF);

    EXPECT_TRUE(OpenError(buffer).Contains("checksum"));
}

TEST(BinaryArchiveTest, FailedFileHeaderBlocksPayloadReads)
{
    const Array<u8> buffer = MakeFileBuffer();

    BinaryFileReader reader(buffer, TypeId::Of<f32>(), TEST_SCHEMA_HASH);

    i64 value = 999; // 센티넬 - 헤더 검증이 실패했으므로 바뀌면 안 됩니다.
    reader.Int(value, EIntWidth::Bits32, true);

    EXPECT_TRUE(reader.HasError());
    EXPECT_EQ(value, 999);
}
