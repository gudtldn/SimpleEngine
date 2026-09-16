#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"


// 층2 ValueOps가 등록 시점에 올바르게 구워지는지, 타입 소거 상태로 컨테이너를 조작할 수 있는지 검증합니다.
namespace se_value_ops_test
{
using namespace se;

/** 기본 생성이 불가능한 타입 — 등록되어도 컴파일 에러가 나면 안 됩니다. */
struct NoDefault
{
    explicit NoDefault(i32 in_value) : value(in_value) {}
    i32 value;
};

/** 타입 T를 등록하고 그 ValueOps를 가져옵니다. */
template <typename T>
const ValueOps& OpsOf()
{
    EnsureRegistered<T>();
    return ValueOpsRegistry::Get().Find(TypeId::Of<T>()).Value();
}

/** for_each로 수집하기 위한 컨텍스트 */
struct CollectContext
{
    Array<i32> numbers;
    Array<String> texts;
};
} // namespace se_value_ops_test

SE_DECLARE_REFLECTION(se_value_ops_test::NoDefault)

SE_REFLECT_BEGIN(se_value_ops_test::NoDefault)
    SE_FIELD(value)
SE_REFLECT_END()


TEST(ValueOpsTest, ArrayOpsAreInstalled)
{
    using namespace se_value_ops_test;

    const se::ValueOps& ops = OpsOf<se::Array<i32>>();
    const se::Optional<const se::ArrayOps&> array_ops = ops.AsArray();
    ASSERT_TRUE(array_ops.HasValue());

    EXPECT_TRUE(array_ops->element_trivially_copyable);
    EXPECT_FALSE(array_ops->element_is_pointer);
    EXPECT_NE(array_ops->resize, nullptr);
    EXPECT_NE(array_ops->resize_uninitialized, nullptr); // i32는 trivially default constructible

    se::Array<i32> numbers;
    array_ops->resize(&numbers, 3);
    ASSERT_EQ(array_ops->len(&numbers), 3u);

    // 타입 소거 상태로 쓴 값이 직접 읽은 값과 같아야 합니다.
    *static_cast<i32*>(array_ops->element_at(&numbers, 0)) = 10;
    *static_cast<i32*>(array_ops->element_at(&numbers, 1)) = 20;
    *static_cast<i32*>(array_ops->element_at(&numbers, 2)) = 30;

    EXPECT_EQ(numbers[0], 10);
    EXPECT_EQ(numbers[1], 20);
    EXPECT_EQ(numbers[2], 30);
    EXPECT_EQ(array_ops->data(&numbers), numbers.Data());
}

TEST(ValueOpsTest, NonTrivialElementDisablesUninitializedResize)
{
    using namespace se_value_ops_test;

    const se::ValueOps& ops = OpsOf<se::Array<se::String>>();
    const se::Optional<const se::ArrayOps&> array_ops = ops.AsArray();
    ASSERT_TRUE(array_ops.HasValue());

    EXPECT_FALSE(array_ops->element_trivially_copyable);
    EXPECT_NE(array_ops->resize, nullptr);
    EXPECT_EQ(array_ops->resize_uninitialized, nullptr);
}

TEST(ValueOpsTest, FixedArrayHasNoResize)
{
    using namespace se_value_ops_test;

    const se::ValueOps& ops = OpsOf<se::FixedArray<i32, 4>>();
    const se::Optional<const se::ArrayOps&> array_ops = ops.AsArray();
    ASSERT_TRUE(array_ops.HasValue());

    EXPECT_EQ(array_ops->resize, nullptr);

    se::FixedArray<i32, 4> fixed;
    EXPECT_EQ(array_ops->len(&fixed), 4u);
    EXPECT_EQ(array_ops->data(&fixed), fixed.Data());
}

TEST(ValueOpsTest, SetOpsInsertAndIterate)
{
    using namespace se_value_ops_test;

    const se::ValueOps& ops = OpsOf<se::HashSet<se::String>>();
    const se::Optional<const se::SetOps&> set_ops = ops.AsSet();
    ASSERT_TRUE(set_ops.HasValue());
    ASSERT_NE(set_ops->emplace_moved, nullptr);

    se::HashSet<se::String> texts;
    se::String alpha = "alpha";
    se::String beta = "beta";
    set_ops->emplace_moved(&texts, &alpha);
    set_ops->emplace_moved(&texts, &beta);

    EXPECT_EQ(set_ops->len(&texts), 2u);

    CollectContext context;
    set_ops->for_each(&texts, [](const void* element, void* user_data)
    {
        static_cast<CollectContext*>(user_data)->texts.Push(*static_cast<const se::String*>(element));
    }, &context);

    EXPECT_EQ(context.texts.Len(), 2u);

    set_ops->clear(&texts);
    EXPECT_EQ(set_ops->len(&texts), 0u);
}

TEST(ValueOpsTest, MapOpsInsertAndIterate)
{
    using namespace se_value_ops_test;

    const se::ValueOps& ops = OpsOf<se::HashMap<se::String, i32>>();
    const se::Optional<const se::MapOps&> map_ops = ops.AsMap();
    ASSERT_TRUE(map_ops.HasValue());
    ASSERT_NE(map_ops->emplace_moved, nullptr);

    se::HashMap<se::String, i32> scores;
    se::String key = "score";
    i32 value = 42;
    map_ops->emplace_moved(&scores, &key, &value);

    EXPECT_EQ(map_ops->len(&scores), 1u);

    CollectContext context;
    map_ops->for_each(&scores, [](const void* map_key, void* map_value, void* user_data)
    {
        auto* collected = static_cast<CollectContext*>(user_data);
        collected->texts.Push(*static_cast<const se::String*>(map_key));
        collected->numbers.Push(*static_cast<i32*>(map_value));
    }, &context);

    ASSERT_EQ(context.numbers.Len(), 1u);
    EXPECT_EQ(context.numbers[0], 42);
    EXPECT_EQ(context.texts[0], "score");
}

TEST(ValueOpsTest, OptionalOpsRoundTrip)
{
    using namespace se_value_ops_test;

    const se::ValueOps& ops = OpsOf<se::Optional<f32>>();
    const se::Optional<const se::OptionalOps&> optional_ops = ops.AsOptional();
    ASSERT_TRUE(optional_ops.HasValue());
    ASSERT_NE(optional_ops->emplace, nullptr);

    se::Optional<f32> maybe_health;
    EXPECT_FALSE(optional_ops->has_value(&maybe_health));

    *static_cast<f32*>(optional_ops->emplace(&maybe_health)) = 75.0f;
    ASSERT_TRUE(optional_ops->has_value(&maybe_health));
    EXPECT_FLOAT_EQ(*static_cast<f32*>(optional_ops->value(&maybe_health)), 75.0f);
    EXPECT_FLOAT_EQ(maybe_health.Value(), 75.0f);

    optional_ops->reset(&maybe_health);
    EXPECT_FALSE(optional_ops->has_value(&maybe_health));
}

TEST(ValueOpsTest, ConstructAndDestructAreInstalledForPlainTypes)
{
    using namespace se_value_ops_test;

    const se::ValueOps& ops = OpsOf<i32>();
    ASSERT_NE(ops.default_construct_at, nullptr);
    ASSERT_NE(ops.destruct_at, nullptr);

    // 컨테이너가 아니므로 형태별 연산은 없습니다.
    EXPECT_FALSE(ops.AsArray().HasValue());
    EXPECT_FALSE(ops.AsSet().HasValue());
    EXPECT_FALSE(ops.AsMap().HasValue());
    EXPECT_FALSE(ops.AsOptional().HasValue());

    alignas(i32) u8 storage[sizeof(i32)];
    ops.default_construct_at(storage);
    EXPECT_EQ(*reinterpret_cast<i32*>(storage), 0);
    ops.destruct_at(storage);
}

TEST(ValueOpsTest, NonDefaultConstructibleTypeStillRegisters)
{
    using namespace se_value_ops_test;

    // 기본 생성이 불가능해도 등록 자체는 성공해야 하고, 해당 op만 nullptr이어야 합니다.
    const se::ValueOps& ops = OpsOf<NoDefault>();
    EXPECT_EQ(ops.default_construct_at, nullptr);
    EXPECT_NE(ops.destruct_at, nullptr);
}
