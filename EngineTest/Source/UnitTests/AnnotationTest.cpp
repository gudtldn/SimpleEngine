#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/RegistrationTraits.h"

#include <algorithm>


// AnnotationList의 조회와, 타입 어노테이션의 RegistrationTraits 훅 호출을 검증합니다.
namespace se_annotation_test
{
using namespace se;

struct Character
{
    SE_ANNOTATE(health, display::Range(0.0f, 100.0f), display::DisplayName("Health Points"))
    f32 health = 100.0f;

    SE_ANNOTATE(id, display::ReadOnly)
    i32 id = 0;

    i32 level = 1;
};

/** OnTypeRegistered가 받은 값을 확인하기 위한 테스트 전용 어노테이션 */
struct CountedAnnotation
{
    i32 tag = 0;
};
consteval CountedAnnotation Counted(i32 tag)
{
    return { .tag = tag };
}

struct CountedItem
{
    i32 value = 0;
};

/** OnTypeRegistered 호출 한 번의 기록 */
struct RegisteredCall
{
    TypeId type;
    i32 tag = 0;
    bool was_registered = false;
};

/** 정적 초기화 중에도 쓸 수 있도록 함수 안의 static으로 둡니다. */
Array<RegisteredCall>& RegisteredCalls()
{
    static Array<RegisteredCall> calls;
    return calls;
}
} // namespace se_annotation_test

template <>
struct se::RegistrationTraits<se_annotation_test::CountedAnnotation>
{
    template <typename T>
    static void OnTypeRegistered(const se_annotation_test::CountedAnnotation& annotation)
    {
        se_annotation_test::RegisteredCalls().Push({
            .type = TypeId::Of<T>(),
            .tag = annotation.tag,
            .was_registered = TypeRegistry::Get().Find(TypeId::Of<T>()).HasValue(),
        });
    }
};

SE_DECLARE_REFLECTION(se_annotation_test::Character)
SE_DECLARE_REFLECTION(se_annotation_test::CountedItem)

SE_REFLECT_BEGIN(se_annotation_test::Character, se::display::Hidden)
    SE_FIELD(health)
    SE_FIELD(id)
    SE_FIELD(level)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_annotation_test::CountedItem, se_annotation_test::Counted(7))
    SE_FIELD(value)
SE_REFLECT_END()


TEST(AnnotationTest, FieldAnnotationValuesAreFound)
{
    using namespace se_annotation_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Character>());
    const auto shape = info.AsStruct();
    ASSERT_TRUE(shape.HasValue());
    ASSERT_EQ(shape->fields.Len(), 3u);

    const se::AnnotationList& health = shape->fields[0].annotations;
    const auto range = health.Find<se::display::RangeAnnotation>();
    ASSERT_TRUE(range.HasValue());
    EXPECT_FLOAT_EQ(range->min, 0.0f);
    EXPECT_FLOAT_EQ(range->max, 100.0f);

    const auto display_name = health.Find<se::display::DisplayNameAnnotation>();
    ASSERT_TRUE(display_name.HasValue());
    EXPECT_EQ(display_name->value, "Health Points");

    EXPECT_FALSE(health.Has<se::display::ReadOnlyAnnotation>());
    EXPECT_TRUE(shape->fields[1].annotations.Has<se::display::ReadOnlyAnnotation>());
}

TEST(AnnotationTest, FieldWithoutAnnotationsFindsNothing)
{
    using namespace se_annotation_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Character>());
    const se::AnnotationList& level = info.AsStruct()->fields[2].annotations;

    EXPECT_EQ(level.refs.Len(), 0u);
    EXPECT_FALSE(level.Has<se::display::RangeAnnotation>());
    EXPECT_FALSE(level.Find<se::display::DisplayNameAnnotation>().HasValue());
}

TEST(AnnotationTest, TypeAnnotationIsFound)
{
    using namespace se_annotation_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Character>());
    EXPECT_TRUE(info.annotations.Has<se::display::HiddenAnnotation>());
    EXPECT_FALSE(info.annotations.Has<se::display::ReadOnlyAnnotation>());
}

TEST(AnnotationTest, OnTypeRegisteredRunsOnceAfterRegistrationWithAnnotationValue)
{
    using namespace se_annotation_test;

    // 다시 등록을 요청해도 훅은 다시 불리지 않아야 함
    se::EnsureRegistered<CountedItem>();

    const auto is_counted_item = [](const RegisteredCall& call)
    {
        return call.type == se::TypeId::Of<CountedItem>();
    };
    ASSERT_EQ(std::ranges::count_if(RegisteredCalls(), is_counted_item), 1);

    const RegisteredCall& call = *std::ranges::find_if(RegisteredCalls(), is_counted_item);
    EXPECT_EQ(call.tag, 7);
    EXPECT_TRUE(call.was_registered);
}

TEST(AnnotationTest, OnTypeRegisteredIsNotCalledForOtherAnnotations)
{
    using namespace se_annotation_test;

    // Character에는 CountedAnnotation이 없음
    const bool has_character = std::ranges::any_of(RegisteredCalls(), [](const RegisteredCall& call)
    {
        return call.type == se::TypeId::Of<Character>();
    });
    EXPECT_FALSE(has_character);
}
