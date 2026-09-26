#include "gtest/gtest.h"

#include "TestEventArchive.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Types/Guid.h"

using namespace se;

// Serialize/Deserialize의 Packed 왕복, 노드 호출 순서, 교체 의미, 손상 입력, nullptr op 처리 검증
namespace se_serializer_test
{
/** HashMap의 값이나 중첩 struct 필드로 쓰는 3차원 벡터 */
struct Vector3
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;

    [[nodiscard]] bool operator==(const Vector3&) const = default;
};

/** 중첩 struct 왕복 검증용 */
struct Transform
{
    Vector3 position;
    Vector3 scale;
};

/** 상속 왕복 검증용 베이스 */
struct Entity
{
    i32 id = 0;
};

/** 상속 왕복 검증용 파생 타입 */
struct NamedEntity : Entity
{
    String name;
};

/** 트레이트 Leaf 필드(Guid, String) 왕복 검증용 */
struct HasTraitLeafFields
{
    Guid id;
    String label;
};

/** 부호 있는 정수를 underlying으로 갖는 enum */
enum class SignedLevel : i32
{
    Low = -1,
    Mid = 0,
    High = 1,
};

/** 부호 없는 정수를 underlying으로 갖는 enum. 최상위 비트 값을 포함합니다. */
enum class UnsignedFlag : u32
{
    None = 0,
    HighBit = 0x80000000,
};

/** enum 필드 왕복 검증용 */
struct HasEnumFields
{
    SignedLevel level = SignedLevel::Mid;
    UnsignedFlag flag = UnsignedFlag::None;
};

/** Array<Array<i32>>, HashMap, HashSet, Optional 필드 왕복 검증용 */
struct HasContainerFields
{
    Array<Array<i32>> grid;
    HashMap<String, Vector3> points;
    HashSet<i32> tags;
    Optional<i32> maybe_count;
};

/** Optional 필드 하나만 갖는 타입. None 왕복과 필드 누락 리셋 검증에 씁니다. */
struct HasOptionalField
{
    Optional<i32> value;
};

/** 노드 호출 순서 골든 테스트용 타입. Set/Map은 순회 순서에 기대지 않도록 원소를 1개만 둡니다. */
struct GoldenOrder
{
    i32 first = 0;
    HashSet<i32> tags;
    HashMap<String, i32> scores;
    Optional<i32> missing;
};

/** 컨테이너 교체 의미 검증용 타입 */
struct ReplaceableContainers
{
    Array<i32> numbers;
    HashSet<i32> tags;
    HashMap<String, i32> scores;

    [[nodiscard]] bool operator==(const ReplaceableContainers&) const = default;
};

/** 손상된 입력(잘린 버퍼)의 오류 경로 검증용 원소 타입 */
struct ItemValue
{
    i32 value = 0;
};

/** items[3].value 형태의 오류 경로를 만들기 위한 컨테이너 타입 */
struct HasItemsArray
{
    Array<ItemValue> items;
};

/** 깊이 제한 검증용 재귀 타입 */
struct RecursiveNode
{
    i32 value = 0;
    Array<RecursiveNode> children;
};

/** 기본 생성이 불가능한 원소 타입. nullptr op(Array resize) 검증에 씁니다. */
struct NoDefaultElement
{
    explicit NoDefaultElement(i32 in_value) : value(in_value) {}
    i32 value;
};

/** 기본 생성 불가 원소의 Array 필드를 갖는 타입 */
struct HasNoDefaultArray
{
    Array<NoDefaultElement> items;
};

/** 기본 생성이 불가능한 Set 원소 타입. 빈 Set은 읽히고 원소가 있으면 오류인지 검증에 씁니다. */
struct NoDefaultKey
{
    explicit NoDefaultKey(i32 in_value) : value(in_value) {}
    i32 value;

    [[nodiscard]] bool operator==(const NoDefaultKey&) const = default;
};
} // namespace se_serializer_test

template <>
struct std::hash<se_serializer_test::NoDefaultKey>
{
    [[nodiscard]] usize operator()(const se_serializer_test::NoDefaultKey& key) const noexcept
    {
        return std::hash<i32>{}(key.value);
    }
};

namespace se_serializer_test
{
/** 기본 생성 불가 원소의 HashSet 필드를 갖는 타입 */
struct HasNoDefaultSet
{
    HashSet<NoDefaultKey> keys;
};
} // namespace se_serializer_test

SE_DECLARE_REFLECTION(se_serializer_test::Vector3)
SE_DECLARE_REFLECTION(se_serializer_test::Transform)
SE_DECLARE_REFLECTION(se_serializer_test::Entity)
SE_DECLARE_REFLECTION(se_serializer_test::NamedEntity)
SE_DECLARE_REFLECTION(se_serializer_test::HasTraitLeafFields)
SE_DECLARE_REFLECTION(se_serializer_test::HasEnumFields)
SE_DECLARE_REFLECTION(se_serializer_test::HasContainerFields)
SE_DECLARE_REFLECTION(se_serializer_test::HasOptionalField)
SE_DECLARE_REFLECTION(se_serializer_test::GoldenOrder)
SE_DECLARE_REFLECTION(se_serializer_test::ReplaceableContainers)
SE_DECLARE_REFLECTION(se_serializer_test::ItemValue)
SE_DECLARE_REFLECTION(se_serializer_test::HasItemsArray)
SE_DECLARE_REFLECTION(se_serializer_test::RecursiveNode)
SE_DECLARE_REFLECTION(se_serializer_test::NoDefaultElement)
SE_DECLARE_REFLECTION(se_serializer_test::HasNoDefaultArray)
SE_DECLARE_REFLECTION(se_serializer_test::NoDefaultKey)
SE_DECLARE_REFLECTION(se_serializer_test::HasNoDefaultSet)

SE_REFLECT_BEGIN(se_serializer_test::Vector3)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::Transform)
    SE_FIELD(position)
    SE_FIELD(scale)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::Entity)
    SE_FIELD(id)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::NamedEntity)
    SE_BASE(se_serializer_test::Entity)
    SE_FIELD(name)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::HasTraitLeafFields)
    SE_FIELD(id)
    SE_FIELD(label)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::HasEnumFields)
    SE_FIELD(level)
    SE_FIELD(flag)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::HasContainerFields)
    SE_FIELD(grid)
    SE_FIELD(points)
    SE_FIELD(tags)
    SE_FIELD(maybe_count)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::HasOptionalField)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::GoldenOrder)
    SE_FIELD(first)
    SE_FIELD(tags)
    SE_FIELD(scores)
    SE_FIELD(missing)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::ReplaceableContainers)
    SE_FIELD(numbers)
    SE_FIELD(tags)
    SE_FIELD(scores)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::ItemValue)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::HasItemsArray)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::RecursiveNode)
    SE_FIELD(value)
    SE_FIELD(children)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::NoDefaultElement)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::HasNoDefaultArray)
    SE_FIELD(items)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::NoDefaultKey)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serializer_test::HasNoDefaultSet)
    SE_FIELD(keys)
SE_REFLECT_END()


// --- Packed 왕복 ---

TEST(SerializerTest, NestedStructPackedRoundTrip)
{
    using namespace se_serializer_test;

    Transform original{
        .position = Vector3{ .x = 1.0f, .y = 2.0f, .z = 3.0f },
        .scale = Vector3{ .x = 1.0f, .y = 1.0f, .z = 1.0f },
    };

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    Transform result;
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_EQ(result.position, original.position);
    EXPECT_EQ(result.scale, original.scale);
}

TEST(SerializerTest, NestedArrayPackedRoundTrip)
{
    using namespace se_serializer_test;

    Array<Array<i32>> original;
    original.Push(Array<i32>{ 1, 2, 3 });
    original.Push(Array<i32>{});
    original.Push(Array<i32>{ 4 });

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    Array<Array<i32>> result;
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_EQ(result, original);
}

TEST(SerializerTest, ContainerFieldsPackedRoundTrip)
{
    using namespace se_serializer_test;

    HasContainerFields original{
        .grid = Array<Array<i32>>{ Array<i32>{ 1, 2 }, Array<i32>{}, Array<i32>{ 3 } },
        .points = HashMap<String, Vector3>{ { String("origin"), Vector3{ .x = 0.0f, .y = 0.0f, .z = 0.0f } } },
        .tags = HashSet<i32>{ 5, 6, 7 },
        .maybe_count = 10,
    };

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    HasContainerFields result;
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_EQ(result.grid, original.grid);
    EXPECT_EQ(result.points, original.points);
    EXPECT_EQ(result.tags, original.tags);
    ASSERT_TRUE(result.maybe_count.HasValue());
    EXPECT_EQ(result.maybe_count.Value(), 10);
}

TEST(SerializerTest, OptionalNonePackedRoundTrip)
{
    using namespace se_serializer_test;

    HasOptionalField original; // 기본값 None

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    HasOptionalField result;
    result.value.Emplace(999); // 데이터의 None이 실제로 반영되는지 보기 위해 미리 Some으로 채움
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_FALSE(result.value.HasValue());
}

TEST(SerializerTest, SignedAndUnsignedEnumPackedRoundTrip)
{
    using namespace se_serializer_test;

    HasEnumFields original{
        .level = SignedLevel::Low,
        .flag = UnsignedFlag::HighBit,
    };

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    HasEnumFields result;
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_EQ(result.level, original.level);
    EXPECT_EQ(result.flag, original.flag);
}

TEST(SerializerTest, InheritedFieldsPackedRoundTrip)
{
    using namespace se_serializer_test;

    NamedEntity original;
    original.id = 42;
    original.name = "Player";

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    NamedEntity result;
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_EQ(result.id, original.id);
    EXPECT_EQ(result.name, original.name);
}

TEST(SerializerTest, TraitLeafFieldsPackedRoundTrip)
{
    using namespace se_serializer_test;

    HasTraitLeafFields original{
        .id = Guid::NewGuid(),
        .label = "hello",
    };

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    HasTraitLeafFields result;
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_EQ(result.id, original.id);
    EXPECT_EQ(result.label, original.label);
}


// --- 노드 호출 순서 골든 (EventWriter) ---

TEST(SerializerTest, EventWriterNodeCallOrderGolden)
{
    using namespace se_serializer_test;

    GoldenOrder original{
        .first = 7,
        .tags = HashSet<i32>{ 11 },
        .scores = HashMap<String, i32>{ { String("alpha"), 42 } },
    };

    Array<SerializeEvent> events;
    EventWriter writer(events, /*is_text_format=*/true);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    ASSERT_EQ(events.Len(), 17u);

    EXPECT_NE(std::get_if<EventBeginStruct>(&events[0]), nullptr);

    const auto* field_first = std::get_if<EventField>(&events[1]);
    ASSERT_NE(field_first, nullptr);
    EXPECT_EQ(field_first->name, "first");
    EXPECT_NE(std::get_if<EventInt>(&events[2]), nullptr);

    const auto* field_tags = std::get_if<EventField>(&events[3]);
    ASSERT_NE(field_tags, nullptr);
    EXPECT_EQ(field_tags->name, "tags");
    const auto* begin_seq = std::get_if<EventBeginSeq>(&events[4]);
    ASSERT_NE(begin_seq, nullptr);
    EXPECT_EQ(begin_seq->count, 1u);
    EXPECT_EQ(begin_seq->order, ESeqOrder::Unordered);
    EXPECT_NE(std::get_if<EventInt>(&events[5]), nullptr);
    EXPECT_NE(std::get_if<EventEndSeq>(&events[6]), nullptr);

    const auto* field_scores = std::get_if<EventField>(&events[7]);
    ASSERT_NE(field_scores, nullptr);
    EXPECT_EQ(field_scores->name, "scores");
    const auto* begin_map = std::get_if<EventBeginMap>(&events[8]);
    ASSERT_NE(begin_map, nullptr);
    EXPECT_EQ(begin_map->count, 1u);
    EXPECT_NE(std::get_if<EventBeginMapEntry>(&events[9]), nullptr);
    EXPECT_NE(std::get_if<EventStr>(&events[10]), nullptr);
    EXPECT_NE(std::get_if<EventInt>(&events[11]), nullptr);
    EXPECT_NE(std::get_if<EventEndMapEntry>(&events[12]), nullptr);
    EXPECT_NE(std::get_if<EventEndMap>(&events[13]), nullptr);

    const auto* field_missing = std::get_if<EventField>(&events[14]);
    ASSERT_NE(field_missing, nullptr);
    EXPECT_EQ(field_missing->name, "missing");
    const auto* present = std::get_if<EventPresent>(&events[15]);
    ASSERT_NE(present, nullptr);
    EXPECT_FALSE(present->value);

    EXPECT_NE(std::get_if<EventEndStruct>(&events[16]), nullptr);
}


// --- 교체 의미 ---

TEST(SerializerTest, DeserializeReplacesExistingContainerContents)
{
    using namespace se_serializer_test;

    ReplaceableContainers source{
        .numbers = Array<i32>{ 1, 2, 3 },
        .tags = HashSet<i32>{ 10, 20 },
        .scores = HashMap<String, i32>{ { String("a"), 1 }, { String("b"), 2 } },
    };

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, source).HasValue());

    // 원본과 다른 원소를 미리 채움. 결과는 누적 없이 데이터와 같아야 함
    ReplaceableContainers target{
        .numbers = Array<i32>{ 99, 98, 97, 96, 95 },
        .tags = HashSet<i32>{ 1000, 2000, 3000 },
        .scores = HashMap<String, i32>{ { String("stale"), -1 } },
    };

    PackedReader reader(buffer);
    ASSERT_TRUE(Deserialize(reader, target).HasValue());

    EXPECT_EQ(target, source);
}


// --- Optional 필드 누락 (EventReader) ---

TEST(SerializerTest, MissingOptionalFieldResetsToNoneEvenWhenPreviouslySome)
{
    using namespace se_serializer_test;

    // "value" 필드가 없는 스트림. EventReader의 Field()는 다음 이벤트가 Field가 아니면 오류 없이 false
    Array<SerializeEvent> events;
    events.Push(SerializeEvent{ EventBeginStruct{} });
    events.Push(SerializeEvent{ EventEndStruct{} });

    EventReader reader(events, /*is_text_format=*/true);

    HasOptionalField target;
    target.value.Emplace(123); // 역직렬화 전에는 Some(123)

    ASSERT_TRUE(Deserialize(reader, target).HasValue());
    EXPECT_FALSE(target.value.HasValue());
}


// --- 손상 입력 ---

TEST(SerializerTest, TruncatedBufferProducesElementFieldPath)
{
    using namespace se_serializer_test;

    HasItemsArray original;
    for (i32 i = 0; i < 5; ++i)
    {
        original.items.Push(ItemValue{ .value = i });
    }

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    // count(4바이트) + item0..2(각 4바이트) = 16바이트 뒤, item3의 4바이트 중 2바이트만 남기고 자름
    buffer.Truncate(18);

    PackedReader reader(buffer);
    HasItemsArray result;
    const auto read_result = Deserialize(reader, result);

    ASSERT_TRUE(read_result.HasError());
    EXPECT_EQ(read_result.Error().path, "items[3].value");
}

TEST(SerializerTest, HugeSeqCountProducesError)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginSeq(999999999, ESeqOrder::Ordered); // count만 쓰고 원소는 쓰지 않음

    PackedReader reader(buffer);
    Array<i32> target;
    const auto read_result = Deserialize(reader, target);

    EXPECT_TRUE(read_result.HasError());
}

TEST(SerializerTest, DeepNestingHitsDepthLimitWithoutCrashing)
{
    using namespace se_serializer_test;

    Array<u8> buffer;
    PackedWriter writer(buffer);
    for (i32 i = 0; i < 300; ++i)
    {
        writer.Int(i, EIntWidth::Bits32, true); // RecursiveNode::value
        writer.BeginSeq(1, ESeqOrder::Ordered);  // RecursiveNode::children (원소 1개)
    }

    PackedReader reader(buffer);
    RecursiveNode result;
    const auto read_result = Deserialize(reader, result);

    ASSERT_TRUE(read_result.HasError());
    EXPECT_TRUE(read_result.Error().message.Contains("depth"));
}


// --- nullptr op ---

TEST(SerializerTest, ArrayOfNonDefaultConstructibleElementFailsToResizeOnDeserialize)
{
    using namespace se_serializer_test;

    HasNoDefaultArray original;
    original.items.Push(NoDefaultElement(1));
    original.items.Push(NoDefaultElement(2));

    Array<u8> buffer;
    PackedWriter writer(buffer);
    const auto write_result = Serialize(writer, original);
    ASSERT_TRUE(write_result.HasValue()); // 쓰기는 len/element_at만 필요

    PackedReader reader(buffer);
    HasNoDefaultArray target; // 빈 배열(길이 0)
    const auto read_result = Deserialize(reader, target);

    EXPECT_TRUE(read_result.HasError()); // 길이(0 -> 2)를 바꿀 수 없어 오류
}

TEST(SerializerTest, EmptySetOfNonDefaultConstructibleElementDeserializes)
{
    using namespace se_serializer_test;

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, HasNoDefaultSet{}).HasValue());

    PackedReader reader(buffer);
    HasNoDefaultSet target;
    EXPECT_TRUE(Deserialize(reader, target).HasValue()); // 원소가 없으면 임시 원소를 만들지 않음
}

TEST(SerializerTest, SetOfNonDefaultConstructibleElementFailsToDeserialize)
{
    using namespace se_serializer_test;

    HasNoDefaultSet original;
    ASSERT_TRUE(original.keys.Emplace(1));

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    HasNoDefaultSet target;
    const auto read_result = Deserialize(reader, target);

    ASSERT_TRUE(read_result.HasError()); // 임시 원소를 기본 생성할 수 없어 오류
    EXPECT_EQ(read_result.Error().path, "keys");
}


// --- PackedFileWriter / PackedFileReader ---

TEST(SerializerTest, PackedFileRoundTrip)
{
    using namespace se_serializer_test;

    const Transform original{
        .position = Vector3{ .x = 1.0f, .y = 2.0f, .z = 3.0f },
        .scale = Vector3{ .x = 2.0f, .y = 2.0f, .z = 2.0f },
    };
    const SerializePlan& plan = SerializePlan::Of<Transform>();

    Array<u8> buffer;
    PackedFileWriter writer(buffer, plan.type, plan.SchemaHash());
    ASSERT_TRUE(Serialize(writer, original).HasValue());
    writer.Finish();

    PackedFileReader reader(buffer, plan.type, plan.SchemaHash());
    Transform result;
    ASSERT_TRUE(Deserialize(reader, result).HasValue());

    EXPECT_EQ(result.position, original.position);
    EXPECT_EQ(result.scale, original.scale);
}

TEST(SerializerTest, PackedFileHeaderMismatchSurfacesAsDeserializeError)
{
    using namespace se_serializer_test;

    const SerializePlan& written_plan = SerializePlan::Of<Transform>();
    Array<u8> buffer;
    PackedFileWriter writer(buffer, written_plan.type, written_plan.SchemaHash());
    ASSERT_TRUE(Serialize(writer, Transform{}).HasValue());
    writer.Finish();

    // Transform으로 쓴 데이터를 다른 타입으로 열면 헤더 검증이 실패하고, 그 오류가 Deserialize의 결과가 됨
    const SerializePlan& read_plan = SerializePlan::Of<HasOptionalField>();
    PackedFileReader reader(buffer, read_plan.type, read_plan.SchemaHash());

    HasOptionalField target{ .value = 5 };
    const auto read_result = Deserialize(reader, target);

    ASSERT_TRUE(read_result.HasError());
    EXPECT_TRUE(read_result.Error().message.Contains("root type"));
    EXPECT_EQ(read_result.Error().path, "");
    ASSERT_TRUE(target.value.HasValue());
    EXPECT_EQ(*target.value, 5); // 헤더에서 멈췄으므로 value는 그대로
}
