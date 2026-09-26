#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"
#include "SimpleEngine/Core/Serialization/SerializePlan.h"
#include "SimpleEngine/Core/Serialization/SerializeTraits.h"
#include "SimpleEngine/Core/Types/HashDigest.h"

#include <algorithm>
#include <cstddef>
#include <string_view>


// SerializePlan의 트레이트 우선, 베이스 평탄화, 재귀, 롤백, 금지 타입 처리 검증
namespace se_serialize_plan_test
{
using namespace se;

/** 평탄화: 베이스 필드가 먼저 오고, 오프셋이 offsetof와 같아야 합니다. */
struct FlattenBase
{
    i32 a = 0;
    i32 b = 0;
};

struct FlattenDerived : FlattenBase
{
    i32 c = 0;
};

/** 이름 충돌: Derived::a가 Base::a를 가립니다. */
struct ShadowBase
{
    i32 a = 0;
    i32 b = 0;
};

struct ShadowDerived : ShadowBase
{
    i32 a = 0;
};

/** 트레이트를 등록한 Struct 타입과, 그 타입을 베이스로 둔 파생 타입입니다. */
struct TraitedLeaf
{
    i32 dummy = 0;
};

struct DerivedFromTraited : TraitedLeaf
{
    i32 extra = 0;
};

/** 트레이트 없는 Opaque 타입을 베이스로 둔 파생 타입입니다. */
struct DerivedFromOpaque : se::HashDigest<16>
{
    i32 extra = 0;
};

/** 등록하지 않는 타입입니다. 이 타입은 EnsureRegistered하지 않습니다. */
struct NeverRegistered
{
    i32 value = 0;
};

/** 트레이트 없는 Opaque 필드를 가진 타입입니다. */
struct HasUntraitedOpaqueField
{
    se::HashDigest<16> hash;
    i32 value = 0;
};

/** 직렬화가 금지된 산술 타입 필드를 가진 타입입니다. */
struct HasWCharField
{
    wchar_t letter = L'\0';
    i32 value = 0;
};

struct HasLongDoubleField
{
    long double precise = 0.0L;
    i32 value = 0;
};

/** underlying이 정수가 아닌 enum입니다. */
enum class BoolBackedEnum : bool
{
    False,
    True,
};

struct HasBoolEnumField
{
    BoolBackedEnum flag = BoolBackedEnum::False;
    i32 value = 0;
};

/** 자기 자신을 원소로 갖는 재귀 타입입니다. */
struct RecursivePlanNode
{
    i32 value = 0;
    se::Array<RecursivePlanNode> children;
};

// 스키마 해시 테스트에서 직접 만든 Plan에 붙이는 TypeId용 태그입니다. 리플렉션에는 등록하지 않습니다.
struct HandStruct {};
struct HandTrait {};
struct HandEnum {};
struct HandNode {};
struct HandNodeArray {};
struct HandOwner {};
struct HandOwnerOptional {};

/** 산술 타입이나 트레이트의 Leaf Plan을 만듭니다. */
[[nodiscard]] SerializePlan MakeLeafPlan(TypeId id, u32 format_version = 0)
{
    return SerializePlan{ .type = id, .steps = LeafStep{ .ops = SerializeOps{ .format_version = format_version } } };
}

/** fields를 가진 Struct Plan을 만듭니다. */
[[nodiscard]] SerializePlan MakeStructPlan(TypeId id, ArrayView<const FieldStep> fields)
{
    return SerializePlan{ .type = id, .steps = StructSteps{ .fields = fields } };
}
} // namespace se_serialize_plan_test

SE_DECLARE_REFLECTION(se_serialize_plan_test::FlattenBase)
SE_DECLARE_REFLECTION(se_serialize_plan_test::FlattenDerived)
SE_DECLARE_REFLECTION(se_serialize_plan_test::ShadowBase)
SE_DECLARE_REFLECTION(se_serialize_plan_test::ShadowDerived)
SE_DECLARE_REFLECTION(se_serialize_plan_test::TraitedLeaf)
SE_DECLARE_REFLECTION(se_serialize_plan_test::DerivedFromTraited)
SE_DECLARE_REFLECTION(se_serialize_plan_test::DerivedFromOpaque)
SE_DECLARE_REFLECTION(se_serialize_plan_test::HasUntraitedOpaqueField)
SE_DECLARE_REFLECTION(se_serialize_plan_test::HasWCharField)
SE_DECLARE_REFLECTION(se_serialize_plan_test::HasLongDoubleField)
SE_DECLARE_REFLECTION(se_serialize_plan_test::HasBoolEnumField)
SE_DECLARE_REFLECTION(se_serialize_plan_test::RecursivePlanNode)

/** 이 파일만의 테스트용 트레이트입니다. Plan 컴파일만 검증하므로 본문은 비워 둡니다. */
template <>
struct se::SerializeTraits<se_serialize_plan_test::TraitedLeaf>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(se::ArchiveWriter&, const se_serialize_plan_test::TraitedLeaf&) {}
    static void Read(se::ArchiveReader&, se_serialize_plan_test::TraitedLeaf&) {}
};

SE_REFLECT_BEGIN(se_serialize_plan_test::FlattenBase)
    SE_FIELD(a)
    SE_FIELD(b)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::FlattenDerived)
    SE_BASE(se_serialize_plan_test::FlattenBase)
    SE_FIELD(c)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::ShadowBase)
    SE_FIELD(a)
    SE_FIELD(b)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::ShadowDerived)
    SE_BASE(se_serialize_plan_test::ShadowBase)
    SE_FIELD(a)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::TraitedLeaf)
    SE_FIELD(dummy)
SE_REFLECT_END()

SE_REGISTER_SERIALIZE_TRAITS(se_serialize_plan_test::TraitedLeaf)

SE_REFLECT_BEGIN(se_serialize_plan_test::DerivedFromTraited)
    SE_BASE(se_serialize_plan_test::TraitedLeaf)
    SE_FIELD(extra)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::DerivedFromOpaque)
    SE_BASE(se::HashDigest<16>)
    SE_FIELD(extra)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::HasUntraitedOpaqueField)
    SE_FIELD(hash)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::HasWCharField)
    SE_FIELD(letter)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::HasLongDoubleField)
    SE_FIELD(precise)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::HasBoolEnumField)
    SE_FIELD(flag)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_serialize_plan_test::RecursivePlanNode)
    SE_FIELD(value)
    SE_FIELD(children)
SE_REFLECT_END()


TEST(SerializePlanTest, FlattensBaseFieldsBeforeOwnFields)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<FlattenDerived>());
    ASSERT_TRUE(result.HasValue());

    const se::StructSteps* steps = std::get_if<se::StructSteps>(&result.Value()->steps);
    ASSERT_NE(steps, nullptr);
    ASSERT_EQ(steps->fields.Len(), 3u);

    EXPECT_EQ(steps->fields[0].name, se::StringView("a"));
    EXPECT_EQ(steps->fields[0].offset, offsetof(FlattenDerived, a));
    EXPECT_EQ(steps->fields[1].name, se::StringView("b"));
    EXPECT_EQ(steps->fields[1].offset, offsetof(FlattenDerived, b));
    EXPECT_EQ(steps->fields[2].name, se::StringView("c"));
    EXPECT_EQ(steps->fields[2].offset, offsetof(FlattenDerived, c));
}

TEST(SerializePlanTest, DuplicateFieldNameAfterFlatteningIsError)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<ShadowDerived>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("duplicate field name 'a'")) << result.Error().CStr();
}

TEST(SerializePlanTest, TraitTakesPriorityOverStructShape)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<TraitedLeaf>());
    ASSERT_TRUE(result.HasValue());

    const se::LeafStep* leaf = std::get_if<se::LeafStep>(&result.Value()->steps);
    ASSERT_NE(leaf, nullptr);

    // Struct로 펼치지 않고 등록된 트레이트 함수를 그대로 쓰는지 확인
    const auto registered = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<TraitedLeaf>());
    ASSERT_TRUE(registered.HasValue());
    EXPECT_EQ(leaf->ops.write, registered->write);
    EXPECT_EQ(leaf->ops.read, registered->read);
}

TEST(SerializePlanTest, BaseWithTraitIsError)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<DerivedFromTraited>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("has a registered SerializeTraits")) << result.Error().CStr();
}

TEST(SerializePlanTest, BaseThatIsNotAStructIsError)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<DerivedFromOpaque>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("is not a struct")) << result.Error().CStr();
}

TEST(SerializePlanTest, UnregisteredTypeIdProducesError)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<NeverRegistered>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("type is not registered")) << result.Error().CStr();
}

TEST(SerializePlanTest, FieldOfUntraitedOpaqueTypeIsError)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<HasUntraitedOpaqueField>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("field 'hash'")) << result.Error().CStr();
    EXPECT_TRUE(result.Error().Contains("no SerializeTraits registered for this opaque type")) << result.Error().CStr();
}

TEST(SerializePlanTest, WCharFieldIsRejected)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<HasWCharField>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("wchar_t has a platform-defined width")) << result.Error().CStr();
}

TEST(SerializePlanTest, LongDoubleFieldIsRejected)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<HasLongDoubleField>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("long double has a platform-defined width")) << result.Error().CStr();
}

TEST(SerializePlanTest, NonIntegerEnumUnderlyingIsError)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<HasBoolEnumField>());
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("is not an integer type")) << result.Error().CStr();
}

TEST(SerializePlanTest, RecursiveTypeElementPlanIsSelf)
{
    using namespace se_serialize_plan_test;

    const auto result = se::SerializePlan::TryOf(se::TypeId::Of<RecursivePlanNode>());
    ASSERT_TRUE(result.HasValue());

    const se::SerializePlan* node_plan = result.Value();
    const se::StructSteps* struct_steps = std::get_if<se::StructSteps>(&node_plan->steps);
    ASSERT_NE(struct_steps, nullptr);
    ASSERT_EQ(struct_steps->fields.Len(), 2u);

    const se::FieldStep& children_field = struct_steps->fields[1];
    EXPECT_EQ(children_field.name, se::StringView("children"));

    const se::ArraySteps* array_steps = std::get_if<se::ArraySteps>(&children_field.plan->steps);
    ASSERT_NE(array_steps, nullptr);
    EXPECT_EQ(array_steps->element, node_plan);
}

TEST(SerializePlanTest, FailedCompilationDoesNotPolluteCache)
{
    using namespace se_serialize_plan_test;

    const auto first = se::SerializePlan::TryOf(se::TypeId::Of<HasUntraitedOpaqueField>());
    const auto second = se::SerializePlan::TryOf(se::TypeId::Of<HasUntraitedOpaqueField>());

    ASSERT_TRUE(first.HasError());
    ASSERT_TRUE(second.HasError());
    EXPECT_EQ(first.Error(), second.Error());
}

TEST(SerializePlanTest, EveryRegisteredOpaqueTypeCompilesOrIsKnownException)
{
    // 이 파일의 음성 테스트 타입이 정적 초기화 때 등록하는, 직렬화할 수 없는 Opaque 타입은 제외
    // (HashDigest<16> 필드와 베이스, wchar_t/long double 필드)
    const se::TypeId exceptions[] = {
        se::TypeId::Of<se::HashDigest<16>>(),
        se::TypeId::Of<wchar_t>(),
        se::TypeId::Of<long double>(),
    };

    for (const se::TypeInfo* info : se::TypeRegistry::Get().GetAllTypes())
    {
        if (!info->IsOpaque())
        {
            continue;
        }

        const bool is_exception = std::ranges::any_of(exceptions, [&](const se::TypeId exception_id)
        {
            return exception_id == info->id;
        });
        if (is_exception)
        {
            continue;
        }

        const auto result = se::SerializePlan::TryOf(info->id);
        EXPECT_TRUE(result.HasValue())
            << "Opaque type '" << std::string_view(info->name.Data(), info->name.ByteLen()) << "' cannot be compiled: "
            << (result.HasError() ? result.Error().CStr() : "");
    }
}


// --- 스키마 해시 ---

TEST(SerializePlanTest, SchemaHashChangesWhenFieldsChange)
{
    using namespace se_serialize_plan_test;

    const SerializePlan i32_plan = MakeLeafPlan(TypeId::Of<i32>());
    const SerializePlan f32_plan = MakeLeafPlan(TypeId::Of<f32>());
    const SerializePlan f64_plan = MakeLeafPlan(TypeId::Of<f64>());
    const TypeId id = TypeId::Of<HandStruct>();

    const FieldStep base[] = { { "hp", 0, &i32_plan }, { "speed", 4, &f32_plan } };
    const FieldStep added[] = { { "hp", 0, &i32_plan }, { "speed", 4, &f32_plan }, { "armor", 8, &i32_plan } };
    const FieldStep removed[] = { { "hp", 0, &i32_plan } };
    const FieldStep reordered[] = { { "speed", 4, &f32_plan }, { "hp", 0, &i32_plan } };
    const FieldStep renamed[] = { { "hp", 0, &i32_plan }, { "velocity", 4, &f32_plan } };
    const FieldStep retyped[] = { { "hp", 0, &i32_plan }, { "speed", 8, &f64_plan } };

    const u64 base_hash = MakeStructPlan(id, base).SchemaHash();
    EXPECT_NE(MakeStructPlan(id, added).SchemaHash(), base_hash) << "field added";
    EXPECT_NE(MakeStructPlan(id, removed).SchemaHash(), base_hash) << "field removed";
    EXPECT_NE(MakeStructPlan(id, reordered).SchemaHash(), base_hash) << "fields reordered";
    EXPECT_NE(MakeStructPlan(id, renamed).SchemaHash(), base_hash) << "field renamed";
    EXPECT_NE(MakeStructPlan(id, retyped).SchemaHash(), base_hash) << "field type changed";
}

TEST(SerializePlanTest, SchemaHashIgnoresFieldOffsets)
{
    using namespace se_serialize_plan_test;

    const SerializePlan i32_plan = MakeLeafPlan(TypeId::Of<i32>());
    const SerializePlan f32_plan = MakeLeafPlan(TypeId::Of<f32>());
    const TypeId id = TypeId::Of<HandStruct>();

    const FieldStep tight[] = { { "hp", 0, &i32_plan }, { "speed", 4, &f32_plan } };
    const FieldStep padded[] = { { "hp", 0, &i32_plan }, { "speed", 16, &f32_plan } };

    EXPECT_EQ(MakeStructPlan(id, tight).SchemaHash(), MakeStructPlan(id, padded).SchemaHash());
}

TEST(SerializePlanTest, SchemaHashChangesWhenTraitFormatVersionChanges)
{
    using namespace se_serialize_plan_test;

    const SerializePlan trait_v1 = MakeLeafPlan(TypeId::Of<HandTrait>(), 1);
    const SerializePlan trait_v2 = MakeLeafPlan(TypeId::Of<HandTrait>(), 2);
    const TypeId id = TypeId::Of<HandStruct>();

    const FieldStep uses_v1[] = { { "id", 0, &trait_v1 } };
    const FieldStep uses_v2[] = { { "id", 0, &trait_v2 } };

    EXPECT_NE(MakeStructPlan(id, uses_v1).SchemaHash(), MakeStructPlan(id, uses_v2).SchemaHash());
}

TEST(SerializePlanTest, SchemaHashChangesWhenEnumChanges)
{
    using namespace se_serialize_plan_test;

    const EnumEntry base[] = { { .value = 0, .name = "Low" }, { .value = 1, .name = "High" } };
    const EnumEntry revalued[] = { { .value = 0, .name = "Low" }, { .value = 2, .name = "High" } };
    const EnumEntry renamed[] = { { .value = 0, .name = "Low" }, { .value = 1, .name = "Top" } };

    const auto enum_hash = [](ArrayView<const EnumEntry> entries, EIntWidth width)
    {
        const SerializePlan plan{
            .type = TypeId::Of<HandEnum>(),
            .steps = EnumStep{ .width = width, .is_signed = true, .entries = entries },
        };
        return plan.SchemaHash();
    };

    const u64 base_hash = enum_hash(base, EIntWidth::Bits32);
    EXPECT_NE(enum_hash(revalued, EIntWidth::Bits32), base_hash) << "entry value changed";
    EXPECT_NE(enum_hash(renamed, EIntWidth::Bits32), base_hash) << "entry name changed";
    EXPECT_NE(enum_hash(base, EIntWidth::Bits8), base_hash) << "underlying width changed";
}

TEST(SerializePlanTest, SchemaHashOfMutuallyRecursiveTypesDoesNotDependOnRoot)
{
    using namespace se_serialize_plan_test;

    // Node { Array<Owner> owners; }, Owner { Optional<Node> node; } 모양의 순환 그래프
    SerializePlan node{ .type = TypeId::Of<HandNode>() };
    SerializePlan owners{ .type = TypeId::Of<HandNodeArray>() };
    SerializePlan owner{ .type = TypeId::Of<HandOwner>() };
    SerializePlan maybe_node{ .type = TypeId::Of<HandOwnerOptional>() };

    const FieldStep node_fields[] = { { "owners", 0, &owners } };
    const FieldStep owner_fields[] = { { "node", 0, &maybe_node } };

    node.steps = StructSteps{ .fields = node_fields };
    owners.steps = ArraySteps{ .element = &owner };
    owner.steps = StructSteps{ .fields = owner_fields };
    maybe_node.steps = OptionalSteps{ .inner = &node };

    // 순회 시작점이 달라 방문 순서가 달라도, 정렬한 뒤 서술하므로 값이 같음
    EXPECT_EQ(node.SchemaHash(), owner.SchemaHash());
    EXPECT_EQ(node.SchemaHash(), maybe_node.SchemaHash());
}
