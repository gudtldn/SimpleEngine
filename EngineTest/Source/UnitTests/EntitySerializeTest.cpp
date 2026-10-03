#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"
#include "SimpleEngine/Core/Serialization/SerializeContext.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/ECS/EntityRemapper.h"

#include <string>
#include <string_view>
#include <utility>

using namespace se;

// Entity가 슬롯 번호 대신 EntityRemapper의 영속 ID로 왕복하는지, 끊긴 참조와 context 누락을 어떻게 다루는지 검증
namespace se_entity_serialize_test
{
/** Entity를 struct 필드와 배열 원소로 담는 타입. ParentComponent와 ChildrenComponent를 흉내 냅니다. */
struct HasEntities
{
    Entity parent;
    Array<Entity> children;

    [[nodiscard]] bool operator==(const HasEntities&) const = default;
};
} // namespace se_entity_serialize_test

SE_DECLARE_REFLECTION(se_entity_serialize_test::HasEntities)

SE_REFLECT_BEGIN(se_entity_serialize_test::HasEntities)
    SE_FIELD(parent)
    SE_FIELD(children)
SE_REFLECT_END()


namespace
{
/** i64를 넘는 영속 ID. TOML에는 10진 문자열로 쓰입니다. */
constexpr u64 LARGE_PERSISTENT_ID = 0xF000'0000'0000'0001;

/** TOML 텍스트를 테이블로 파싱합니다. 문법 오류면 테스트를 실패시키고 빈 테이블을 돌려줍니다. */
toml::table ParseToml(std::string_view text)
{
    toml::parse_result result = toml::parse(text);
    if (!result)
    {
        ADD_FAILURE() << "TOML parse error: " << result.error().description();
        return {};
    }
    return std::move(result).table();
}
} // namespace


TEST(EntitySerializeTest, RoundTripMapsToLoadedEntities)
{
    using se_entity_serialize_test::HasEntities;

    // 저장하는 쪽은 저장하는 엔티티마다 영속 ID를 정함
    EntityManager saved_entities;
    const Entity root = saved_entities.Create();
    const Entity child = saved_entities.Create();
    EntityRemapper save_remapper;
    ASSERT_TRUE(save_remapper.Add(root, 7));
    ASSERT_TRUE(save_remapper.Add(child, LARGE_PERSISTENT_ID));
    SerializeContext save_context;
    save_context.Add(save_remapper);

    // 로드하는 쪽은 같은 영속 ID에 새로 만든 엔티티를 짝지음. 슬롯 번호가 원본과 다르도록 하나를 먼저 만듦
    EntityManager loaded_entities;
    loaded_entities.Create();
    const Entity loaded_root = loaded_entities.Create();
    const Entity loaded_child = loaded_entities.Create();
    EntityRemapper load_remapper;
    ASSERT_TRUE(load_remapper.Add(loaded_root, 7));
    ASSERT_TRUE(load_remapper.Add(loaded_child, LARGE_PERSISTENT_ID));
    SerializeContext load_context;
    load_context.Add(load_remapper);

    const HasEntities original{ .parent = root, .children = { child, Entity{}, root } };
    const HasEntities expected{ .parent = loaded_root, .children = { loaded_child, Entity{}, loaded_root } };
    {
        Array<u8> buffer;
        BinaryWriter writer(buffer);
        writer.SetContext(&save_context);
        ASSERT_TRUE(serde::Serialize(writer, original).HasValue());

        BinaryReader reader(buffer);
        reader.SetContext(&load_context);
        HasEntities result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result, expected);
    }
    {
        toml::table table;
        TomlWriter writer(table);
        writer.SetContext(&save_context);
        ASSERT_TRUE(serde::Serialize(writer, original).HasValue());

        // 슬롯 번호가 아니라 영속 ID를 씀
        EXPECT_EQ(table["parent"].value_exact<i64>(), 7);
        EXPECT_EQ(table["children"][0].value_exact<std::string>(), "17293822569102704641");
        EXPECT_EQ(table["children"][1].value_exact<i64>(), 0);

        TomlReader reader(table);
        reader.SetContext(&load_context);
        HasEntities result;
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result, expected);
    }
    EXPECT_TRUE(load_remapper.GetUnresolvedIds().IsEmpty());
}

TEST(EntitySerializeTest, NullEntityIsZero)
{
    using se_entity_serialize_test::HasEntities;

    // null Entity는 짝을 넣지 않아도 0으로 쓰고 읽음
    EntityRemapper remapper;
    SerializeContext context;
    context.Add(remapper);

    toml::table table;
    TomlWriter writer(table);
    writer.SetContext(&context);
    ASSERT_TRUE(serde::Serialize(writer, HasEntities{ .children = { Entity{} } }).HasValue());
    EXPECT_EQ(table["parent"].value_exact<i64>(), 0);
    EXPECT_EQ(table["children"][0].value_exact<i64>(), 0);

    EntityManager entities;
    TomlReader reader(table);
    reader.SetContext(&context);
    HasEntities result{ .parent = entities.Create() };
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_FALSE(result.parent.IsValid());
    EXPECT_EQ(result.children, (Array<Entity>{ Entity{} }));
    EXPECT_TRUE(remapper.GetUnresolvedIds().IsEmpty());
}

TEST(EntitySerializeTest, MissingContextOrRemapperIsError)
{
    using se_entity_serialize_test::HasEntities;

    SerializeContext empty_context;
    struct Case
    {
        SerializeContext* context;
        const char* message;
    };
    const Case cases[] = {
        { nullptr, "SerializeTraits<Entity>: the archive has no SerializeContext. Add an EntityRemapper to a SerializeContext and pass it with SetContext." },
        { &empty_context, "SerializeTraits<Entity>: the SerializeContext has no EntityRemapper." },
    };

    const toml::table table = ParseToml("parent = 0\nchildren = []");
    for (const Case& c : cases)
    {
        Array<u8> buffer;
        BinaryWriter writer(buffer);
        writer.SetContext(c.context);
        const auto write_result = serde::Serialize(writer, HasEntities{});
        ASSERT_TRUE(write_result.HasError());
        EXPECT_EQ(write_result.Error().path, "parent");
        EXPECT_EQ(write_result.Error().message, c.message);

        TomlReader reader(table);
        reader.SetContext(c.context);
        HasEntities result;
        const auto read_result = serde::Deserialize(reader, result);
        ASSERT_TRUE(read_result.HasError());
        EXPECT_EQ(read_result.Error().path, "parent");
        EXPECT_EQ(read_result.Error().message, c.message);
    }
}

TEST(EntitySerializeTest, UnknownEntityIsWrittenAsNullAndRecorded)
{
    using se_entity_serialize_test::HasEntities;

    // 저장하지 않는 엔티티를 가리키는 참조는 null로 쓰고, 같은 엔티티는 한 번만 기록
    EntityManager entities;
    const Entity unsaved = entities.Create();
    const Entity other_unsaved = entities.Create();
    EntityRemapper remapper;
    SerializeContext context;
    context.Add(remapper);

    toml::table table;
    TomlWriter writer(table);
    writer.SetContext(&context);
    ASSERT_TRUE(serde::Serialize(writer, HasEntities{ .parent = unsaved, .children = { unsaved, other_unsaved } }).HasValue());
    EXPECT_EQ(table["parent"].value_exact<i64>(), 0);
    EXPECT_EQ(table["children"][0].value_exact<i64>(), 0);
    EXPECT_EQ(table["children"][1].value_exact<i64>(), 0);

    const ArrayView<const Entity> unresolved = remapper.GetUnresolvedEntities();
    ASSERT_EQ(unresolved.Len(), 2u);
    EXPECT_EQ(unresolved[0], unsaved);
    EXPECT_EQ(unresolved[1], other_unsaved);
    EXPECT_EQ(remapper.GetUnresolvedEntityCount(), 3u);
}

TEST(EntitySerializeTest, UnknownIdIsReadAsNullAndRecorded)
{
    using se_entity_serialize_test::HasEntities;

    EntityManager entities;
    const Entity known = entities.Create();
    EntityRemapper remapper;
    ASSERT_TRUE(remapper.Add(known, 100));
    SerializeContext context;
    context.Add(remapper);

    // 파일에만 있는 ID(999, 555)는 끊긴 참조로 보고 로드는 계속함
    const toml::table table = ParseToml("parent = 999\nchildren = [ 100, 555, 999 ]");
    TomlReader reader(table);
    reader.SetContext(&context);
    HasEntities result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_FALSE(result.parent.IsValid());
    EXPECT_EQ(result.children, (Array<Entity>{ known, Entity{}, Entity{} }));

    // 로더가 알릴 수 있게 처음 만난 순서대로 한 번씩 기록
    const ArrayView<const u64> unresolved = remapper.GetUnresolvedIds();
    ASSERT_EQ(unresolved.Len(), 2u);
    EXPECT_EQ(unresolved[0], 999u);
    EXPECT_EQ(unresolved[1], 555u);
}

TEST(EntitySerializeTest, LoadingTwiceCreatesDistinctEntities)
{
    using se_entity_serialize_test::HasEntities;

    EntityManager saved_entities;
    const Entity root = saved_entities.Create();
    const Entity child = saved_entities.Create();
    EntityRemapper save_remapper;
    ASSERT_TRUE(save_remapper.Add(root, 1));
    ASSERT_TRUE(save_remapper.Add(child, 2));
    SerializeContext save_context;
    save_context.Add(save_remapper);

    Array<u8> buffer;
    BinaryWriter writer(buffer);
    writer.SetContext(&save_context);
    ASSERT_TRUE(serde::Serialize(writer, HasEntities{ .parent = root, .children = { child } }).HasValue());

    // 같은 데이터를 같은 엔티티 풀에 두 번 로드. 로드마다 새 엔티티를 만들어 새 EntityRemapper에 짝지음
    EntityManager world;
    const auto load = [&]() -> HasEntities
    {
        const Entity new_root = world.Create();
        const Entity new_child = world.Create();
        EntityRemapper load_remapper;
        EXPECT_TRUE(load_remapper.Add(new_root, 1));
        EXPECT_TRUE(load_remapper.Add(new_child, 2));
        SerializeContext load_context;
        load_context.Add(load_remapper);

        BinaryReader reader(buffer);
        reader.SetContext(&load_context);
        HasEntities result;
        EXPECT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result, (HasEntities{ .parent = new_root, .children = { new_child } }));
        return result;
    };
    const HasEntities first = load();
    const HasEntities second = load();

    // 두 번째 로드의 참조는 두 번째로 만든 엔티티를 가리킴
    EXPECT_NE(first.parent, second.parent);
    EXPECT_NE(first.children[0], second.children[0]);
    EXPECT_TRUE(world.IsValid(first.parent));
    EXPECT_TRUE(world.IsValid(second.parent));
}

TEST(EntitySerializeTest, RemapperAddRejectsNullZeroAndDuplicates)
{
    EntityManager entities;
    const Entity first = entities.Create();
    const Entity second = entities.Create();

    EntityRemapper remapper;
    EXPECT_FALSE(remapper.Add(Entity{}, 1)); // null Entity
    EXPECT_FALSE(remapper.Add(first, 0));    // 0은 null Entity의 ID
    EXPECT_TRUE(remapper.Add(first, 1));
    EXPECT_FALSE(remapper.Add(second, 1));   // 이미 넣은 ID (파일의 중복 ID 등)
    EXPECT_FALSE(remapper.Add(first, 2));    // 이미 넣은 엔티티

    // 실패한 Add는 아무것도 바꾸지 않음
    EXPECT_EQ(remapper.ToPersistentId(first), u64{ 1 });
    EXPECT_EQ(remapper.ToPersistentId(second), u64{ 0 });
    EXPECT_EQ(remapper.ToEntity(1), first);
    EXPECT_EQ(remapper.ToPersistentId(Entity{}), u64{ 0 });
}
