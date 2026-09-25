#include "gtest/gtest.h"

#include "TestEventArchive.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Serialization/BuiltinTraits.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"

using namespace se;

namespace se_builtin_traits_test
{
// TypeRegistry에 등록하지 않는 테스트 전용 타입입니다. TypeId::Of는 등록을 하지 않으므로 계속 미등록 상태입니다.
// 익명 네임스페이스의 타입은 정규 이름을 만들 수 없어 TypeId를 구할 수 없으므로, 이름 있는 네임스페이스에 둡니다.
struct UnregisteredType {};
} // namespace se_builtin_traits_test


// --- 바이너리(Packed) 왕복 ---

TEST(BuiltinTraitsTest, StringBinaryRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    String original = "Hello, SimpleEngine!";
    SerializeTraits<String>::Write(writer, original);

    PackedReader reader(buffer);
    String result;
    SerializeTraits<String>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, StringNameBinaryRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    StringName original = "PlayerHealth";
    SerializeTraits<StringName>::Write(writer, original);

    PackedReader reader(buffer);
    StringName result;
    SerializeTraits<StringName>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, GuidBinaryRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    Guid original = Guid::NewGuid();
    SerializeTraits<Guid>::Write(writer, original);

    PackedReader reader(buffer);
    Guid result;
    SerializeTraits<Guid>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, TypeIdBinaryRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    TypeId original = TypeId::Of<i32>();
    SerializeTraits<TypeId>::Write(writer, original);

    PackedReader reader(buffer);
    TypeId result;
    SerializeTraits<TypeId>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, UnregisteredTypeIdBinaryRoundTripSucceeds)
{
    // 바이너리는 쓰기가 레지스트리를 요구하지 않으므로 읽기도 요구하지 않습니다(대칭).
    Array<u8> buffer;
    PackedWriter writer(buffer);
    TypeId original = TypeId::Of<se_builtin_traits_test::UnregisteredType>();
    SerializeTraits<TypeId>::Write(writer, original);

    PackedReader reader(buffer);
    TypeId result;
    SerializeTraits<TypeId>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, ContentHashBinaryRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    const u8 raw[32] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    };
    ContentHash original = ContentHash::FromRaw(raw);
    SerializeTraits<ContentHash>::Write(writer, original);

    PackedReader reader(buffer);
    ContentHash result;
    SerializeTraits<ContentHash>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, PathBinaryRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    Path original("Assets/Textures/rock.png");
    SerializeTraits<Path>::Write(writer, original);

    PackedReader reader(buffer);
    Path result;
    SerializeTraits<Path>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, VPathBinaryRoundTrip)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    VPath original("Assets://Textures/rock.png");
    SerializeTraits<VPath>::Write(writer, original);

    PackedReader reader(buffer);
    VPath result;
    SerializeTraits<VPath>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}


// --- 텍스트 포맷(EventWriter/EventReader) 왕복 ---

TEST(BuiltinTraitsTest, GuidTextRoundTrip)
{
    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    Guid original = Guid::NewGuid();
    SerializeTraits<Guid>::Write(writer, original);

    EventReader reader(events, /*is_text_format=*/true);
    Guid result;
    SerializeTraits<Guid>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, ContentHashTextRoundTrip)
{
    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    const u8 raw[32] = {
        32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,
        16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
    };
    ContentHash original = ContentHash::FromRaw(raw);
    SerializeTraits<ContentHash>::Write(writer, original);

    EventReader reader(events, /*is_text_format=*/true);
    ContentHash result;
    SerializeTraits<ContentHash>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}

TEST(BuiltinTraitsTest, TypeIdTextRoundTrip)
{
    // 텍스트 쓰기는 레지스트리 조회가 필요하므로, 먼저 등록을 보장합니다.
    EnsureRegistered<Guid>();

    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    TypeId original = TypeId::Of<Guid>();
    SerializeTraits<TypeId>::Write(writer, original);
    ASSERT_FALSE(writer.HasError());

    EventReader reader(events, /*is_text_format=*/true);
    TypeId result;
    SerializeTraits<TypeId>::Read(reader, result);

    EXPECT_EQ(result, original);
    EXPECT_FALSE(reader.HasError());
}


// --- 오류 경로 ---

TEST(BuiltinTraitsTest, GuidTextReadRejectsInvalidStringWithoutAsserting)
{
    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    writer.Str("not-a-valid-guid");

    EventReader reader(events, /*is_text_format=*/true);
    Guid result;
    SerializeTraits<Guid>::Read(reader, result);

    EXPECT_TRUE(reader.HasError());
}

TEST(BuiltinTraitsTest, ContentHashTextReadRejectsInvalidHexWithoutAsserting)
{
    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    writer.Str("not-a-valid-hex-string");

    EventReader reader(events, /*is_text_format=*/true);
    ContentHash result;
    SerializeTraits<ContentHash>::Read(reader, result);

    EXPECT_TRUE(reader.HasError());
}

TEST(BuiltinTraitsTest, TypeIdTextWriteFailsWhenUnregistered)
{
    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    SerializeTraits<TypeId>::Write(writer, TypeId::Of<se_builtin_traits_test::UnregisteredType>());

    EXPECT_TRUE(writer.HasError());
}

TEST(BuiltinTraitsTest, TypeIdTextReadFailsForUnknownName)
{
    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    writer.Str("NoSuchRegisteredType_ThisNameShouldNeverExist");

    EventReader reader(events, /*is_text_format=*/true);
    TypeId result;
    SerializeTraits<TypeId>::Read(reader, result);

    EXPECT_TRUE(reader.HasError());
}


// --- concept 검사 ---

TEST(BuiltinTraitsTest, HasSerializeTraitsDistinguishesBuiltinTypesFromPlainTypes)
{
    static_assert(HasSerializeTraits<Guid>);
    static_assert(HasSerializeTraits<String>);
    static_assert(HasSerializeTraits<StringName>);
    static_assert(HasSerializeTraits<TypeId>);
    static_assert(HasSerializeTraits<ContentHash>);
    static_assert(HasSerializeTraits<Path>);
    static_assert(HasSerializeTraits<VPath>);
    static_assert(!HasSerializeTraits<i32>);
    static_assert(!HasSerializeTraits<bool>);
    SUCCEED();
}
