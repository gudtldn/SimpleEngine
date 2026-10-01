#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"

#include <cstring>
#include <string_view>
#include <variant>

using namespace se;

// Packed 인코딩이 메모리 바이트와 같은 원소의 배열은 원소마다 쓰지 않고 저장소를 한 번에 쓰고 읽으므로,
// Plan의 판단과, 한 번에 쓴 바이트가 원소마다 쓴 바이트와 같은지, 잘린 데이터를 오류로 처리하는지 검증
namespace se_array_bulk_test
{
/** u8 뒤에 패딩 3바이트가 생기는 구조체 */
struct PaddedPair
{
    u8 tag = 0;
    u32 value = 0;
};

/** 필드를 오프셋과 반대 순서로 등록한 구조체 */
struct ReversedPair
{
    f32 first = 0.0f;
    f32 second = 0.0f;
};
} // namespace se_array_bulk_test

SE_DECLARE_REFLECTION(se_array_bulk_test::PaddedPair)
SE_DECLARE_REFLECTION(se_array_bulk_test::ReversedPair)

SE_REFLECT_BEGIN(se_array_bulk_test::PaddedPair)
    SE_FIELD(tag)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_array_bulk_test::ReversedPair)
    SE_FIELD(second)
    SE_FIELD(first)
SE_REFLECT_END()


namespace
{
/** seed마다 다른 값을 가진 정점을 만듭니다. */
[[nodiscard]] StaticVertex MakeVertex(const f32 seed)
{
    return {
        .position = { seed, seed + 1.0f, seed + 2.0f },
        .normal = { 0.0f, 1.0f, 0.0f },
        .tex_coord = { seed * 0.5f, 0.25f },
        .tangent = { 1.0f, 0.0f, 0.0f, -1.0f },
    };
}

/** 서로 다른 정점 count개를 만듭니다. */
[[nodiscard]] Array<StaticVertex> MakeVertices(const usize count)
{
    Array<StaticVertex> vertices;
    vertices.Reserve(count);
    for (usize i = 0; i < count; ++i)
    {
        vertices.Push(MakeVertex(static_cast<f32>(i)));
    }
    return vertices;
}

/** Container의 Plan이 원소를 한 번에 쓸 때의 원소 바이트 수를 돌려줍니다. 원소마다 쓰면 0입니다. */
template <typename Container>
[[nodiscard]] usize RawElementSizeOf()
{
    return std::get<ArraySteps>(SerializePlanOf<Container>().steps).raw_element_size;
}

/** values를 serde::Serialize로 Packed에 씁니다. */
template <typename T>
[[nodiscard]] Array<u8> WritePacked(const Array<T>& values)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    EXPECT_TRUE(serde::Serialize(writer, values).HasValue());
    return buffer;
}

/** 배열 경로를 거치지 않고, 같은 길이 접두 뒤에 원소를 하나씩 serde::Serialize로 Packed에 씁니다. */
template <typename T>
[[nodiscard]] Array<u8> WriteElementByElement(const Array<T>& values)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginSeq(values.Len(), ESeqOrder::Ordered);
    for (const T& value : values)
    {
        EXPECT_TRUE(serde::Serialize(writer, value).HasValue());
    }
    writer.EndSeq();
    return buffer;
}

/** original을 Packed로 쓰고 다른 내용을 담은 배열에 다시 읽어, 길이와 바이트가 같은지 확인합니다. */
template <typename T>
void ExpectRoundTrip(const Array<T>& original)
{
    SCOPED_TRACE(std::string_view{ TypeNameOf<Array<T>>() });

    const Array<u8> buffer = WritePacked(original);

    PackedReader reader(buffer);
    Array<T> result;
    result.Resize(2);
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    ASSERT_EQ(result.Len(), original.Len());
    EXPECT_TRUE(original.IsEmpty() || std::memcmp(result.Data(), original.Data(), original.Len() * sizeof(T)) == 0);
}
} // namespace


TEST(ArrayBulkSerializeTest, PlanMarksOnlyMemoryIdenticalTypesAsTriviallyPackable)
{
    using namespace se_array_bulk_test;

    EXPECT_TRUE(SerializePlanOf<u32>().is_trivially_packable);
    EXPECT_TRUE(SerializePlanOf<f32>().is_trivially_packable);
    EXPECT_TRUE(SerializePlanOf<StaticVertex>().is_trivially_packable);
    EXPECT_TRUE(SerializePlanOf<MeshSection>().is_trivially_packable);

    // bool은 읽을 때 0/1로 바꾸고, enum은 이름을 가진 값이라 제외
    EXPECT_FALSE(SerializePlanOf<bool>().is_trivially_packable);
    EXPECT_FALSE(SerializePlanOf<EBlendMode>().is_trivially_packable);

    // FixedArray도 길이 접두를 쓰므로 그것을 담은 SkinVertex는 메모리와 다름
    EXPECT_FALSE(SerializePlanOf<SkinVertex>().is_trivially_packable);

    EXPECT_FALSE(SerializePlanOf<PaddedPair>().is_trivially_packable);
    EXPECT_FALSE(SerializePlanOf<ReversedPair>().is_trivially_packable);
}

TEST(ArrayBulkSerializeTest, OnlyArraysOfTriviallyPackableElementsWriteRawElements)
{
    EXPECT_EQ(RawElementSizeOf<Array<StaticVertex>>(), sizeof(StaticVertex));
    EXPECT_EQ(RawElementSizeOf<Array<u32>>(), sizeof(u32));
    EXPECT_EQ(RawElementSizeOf<Array<u8>>(), sizeof(u8));

    EXPECT_EQ(RawElementSizeOf<Array<SkinVertex>>(), 0u);
    EXPECT_EQ(RawElementSizeOf<Array<bool>>(), 0u);
}

TEST(ArrayBulkSerializeTest, VertexEncodingEqualsItsMemoryBytes)
{
    const StaticVertex vertex = MakeVertex(3.0f);

    // 배열이 아닌 구조체 하나는 필드마다 씀
    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(serde::Serialize(writer, vertex).HasValue());

    ASSERT_EQ(buffer.Len(), sizeof(StaticVertex));
    EXPECT_EQ(std::memcmp(buffer.Data(), &vertex, sizeof(StaticVertex)), 0);
}

TEST(ArrayBulkSerializeTest, RawElementsWriteSameBytesAsElementByElement)
{
    const Array<StaticVertex> vertices = MakeVertices(3);
    EXPECT_EQ(WritePacked(vertices), WriteElementByElement(vertices));

    const Array<u32> indices = { 0, 1, 2, 70000, 0xFFFFFFFF };
    EXPECT_EQ(WritePacked(indices), WriteElementByElement(indices));

    const Array<u8> pixels = { 0, 1, 128, 255 };
    EXPECT_EQ(WritePacked(pixels), WriteElementByElement(pixels));
}

TEST(ArrayBulkSerializeTest, PackedRoundTripKeepsEveryByte)
{
    ExpectRoundTrip(MakeVertices(1000));
    ExpectRoundTrip(Array<u32>{ 0, 1, 2, 70000, 0xFFFFFFFF });
    ExpectRoundTrip(Array<u8>{ 0, 1, 128, 255 });

    ExpectRoundTrip(Array<StaticVertex>{});
    ExpectRoundTrip(Array<u32>{});
    ExpectRoundTrip(Array<u8>{});
}

TEST(ArrayBulkSerializeTest, TruncatedElementBytesFailCleanly)
{
    const Array<u8> buffer = WritePacked(MakeVertices(4));

    // 길이 접두 검사는 통과하지만 원소 바이트가 1바이트 모자람
    PackedReader reader(ArrayView<const u8>(buffer.Data(), buffer.Len() - 1));
    Array<StaticVertex> result;
    const auto read_result = serde::Deserialize(reader, result);

    ASSERT_TRUE(read_result.HasError());
    EXPECT_TRUE(read_result.Error().message.Contains("buffer overflow"));
}

TEST(ArrayBulkSerializeTest, CountLargerThanRemainingBytesFailsBeforeResizing)
{
    // 원소 1000개라고 적었지만 뒤에 8바이트만 있는 데이터
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginSeq(1000, ESeqOrder::Ordered);
    constexpr u64 TRAILING_BYTES = 0;
    writer.Bytes(&TRAILING_BYTES, sizeof(TRAILING_BYTES));

    PackedReader reader(buffer);
    Array<StaticVertex> result = MakeVertices(2);
    const auto read_result = serde::Deserialize(reader, result);

    ASSERT_TRUE(read_result.HasError());
    EXPECT_TRUE(read_result.Error().message.Contains("exceeds remaining bytes"));
    EXPECT_EQ(result.Len(), 2u);
}

TEST(ArrayBulkSerializeTest, TextFormatRejectsRawElements)
{
    toml::table table;
    TomlWriter writer(table);
    constexpr u32 VALUE = 7;
    writer.RawElements(&VALUE, sizeof(VALUE));
    EXPECT_TRUE(writer.HasError());

    TomlReader reader(table);
    u32 read_value = 0;
    reader.RawElements(&read_value, sizeof(read_value));
    EXPECT_TRUE(reader.HasError());
}
