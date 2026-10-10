#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/Ignore.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"

#include <cstddef>


// Ignore가 붙은 필드를 등록에서 빼는지 검증합니다.
namespace se_ignore_test
{
using namespace se;

/** 리플렉션에 등록하지 않는 타입 */
struct Unregistered
{
    i32 value = 0;
};

struct Holder
{
    explicit Holder(Unregistered& target)
        : target_ref(target)
    {
    }

    i32 first = 1;

    SE_ANNOTATE(target_ref, Ignore)
    Unregistered& target_ref;

    SE_ANNOTATE(target_ptr, Ignore)
    Unregistered* target_ptr = nullptr;

    SE_ANNOTATE(by_value, Ignore)
    Unregistered by_value;

    f32 last = 2.0f;
};

/** 인스턴스에서 직접 잰 멤버의 바이트 오프셋 */
usize MeasuredOffset(const Holder& holder, const void* member)
{
    return static_cast<usize>(static_cast<const std::byte*>(member) - reinterpret_cast<const std::byte*>(&holder));
}
} // namespace se_ignore_test

SE_DECLARE_REFLECTION(se_ignore_test::Holder)

SE_REFLECT_BEGIN(se_ignore_test::Holder)
    SE_FIELD(first)
    SE_FIELD(target_ref)
    SE_FIELD(target_ptr)
    SE_FIELD(by_value)
    SE_FIELD(last)
SE_REFLECT_END()


TEST(IgnoreAnnotationTest, IgnoredFieldsAreNotRegistered)
{
    using namespace se_ignore_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Holder>());
    const auto shape = info.AsStruct();
    ASSERT_TRUE(shape.HasValue());
    ASSERT_EQ(shape->fields.Len(), 2u);

    EXPECT_EQ(shape->fields[0].name, "first");
    EXPECT_EQ(shape->fields[1].name, "last");
}

TEST(IgnoreAnnotationTest, KeptFieldOffsetsMatchInstanceLayout)
{
    using namespace se_ignore_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Holder>());
    const auto shape = info.AsStruct();
    ASSERT_TRUE(shape.HasValue());
    ASSERT_EQ(shape->fields.Len(), 2u);

    Unregistered target;
    const Holder holder{ target };
    EXPECT_EQ(shape->fields[0].offset, MeasuredOffset(holder, &holder.first));
    EXPECT_EQ(shape->fields[1].offset, MeasuredOffset(holder, &holder.last));
}

TEST(IgnoreAnnotationTest, TypeOfIgnoredFieldIsNotRegistered)
{
    using namespace se_ignore_test;

    (void)se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Holder>());
    EXPECT_FALSE(se::TypeRegistry::Get().Find(se::TypeId::Of<Unregistered>()).HasValue());
}
