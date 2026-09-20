#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Serialization/BuiltinTraits.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"

using namespace se;

TEST(BuiltinTraitsTest, StringRoundTrip)
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

TEST(BuiltinTraitsTest, StringNameRoundTrip)
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

TEST(BuiltinTraitsTest, GuidRoundTrip)
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

TEST(BuiltinTraitsTest, TypeIdRoundTrip)
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

TEST(BuiltinTraitsTest, HashDigestRoundTrip)
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

TEST(BuiltinTraitsTest, PathRoundTrip)
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

TEST(BuiltinTraitsTest, VPathRoundTrip)
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

TEST(BuiltinTraitsTest, HasSerializeTraitsRejectsPlainTypes)
{
    static_assert(HasSerializeTraits<Guid>);
    static_assert(HasSerializeTraits<String>);
    static_assert(HasSerializeTraits<ContentHash>);
    static_assert(!HasSerializeTraits<i32>);
    static_assert(!HasSerializeTraits<bool>);
    SUCCEED();
}
