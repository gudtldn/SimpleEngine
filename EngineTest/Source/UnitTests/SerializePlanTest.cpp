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
