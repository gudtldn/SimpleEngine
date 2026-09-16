#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"

#include <variant>


// P3(프론트엔드 매크로)가 실제로 정적 초기화 시점에 자동 등록을 트리거하는지,
// 어노테이션 왕복, enum의 자동(빈 entries)/명시(SE_REFLECT_ENUM_*) 두 경로를 검증합니다.
namespace se_reflect_macros_golden_test
{
using namespace se;

// 자동 경로 — 아무 매크로도 안 씁니다. Weapon의 필드로 쓰여 전이적으로 등록됩니다.
enum class EAutoColor
{
    Red,
    Green,
    Blue,
};

// 명시 경로 — 런타임에 이름 조회가 필요해서 SE_REFLECT_ENUM_*로 나열합니다.
enum class EWeaponType
{
    Sword,
    Bow,
    Staff,
};

struct RangeTag
{
    f32 min = 0.0f;
    f32 max = 0.0f;
};

struct TooltipTag
{
    StringView text;
};

struct ComponentTag {};

struct Weapon
{
    // SE_CLASS 류의 별도 매크로 없이, decltype(*this)만으로 필드 존재 검증이 동작하는지도 같이 검증합니다.
    SE_ANNOTATE(damage, RangeTag{ 0.0f, 100.0f }, TooltipTag{ "Damage amount" })
    i32 damage = 0;

    EWeaponType type = EWeaponType::Sword;
    EAutoColor tint = EAutoColor::Red;
};
} // namespace se_reflect_macros_golden_test

SE_DECLARE_REFLECTION(se_reflect_macros_golden_test::EWeaponType)
SE_DECLARE_REFLECTION(se_reflect_macros_golden_test::Weapon)

SE_REFLECT_ENUM_BEGIN(se_reflect_macros_golden_test::EWeaponType)
    SE_ENUM_VALUE(Sword)
    SE_ENUM_VALUE(Bow)
    SE_ENUM_VALUE(Staff)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se_reflect_macros_golden_test::Weapon, se_reflect_macros_golden_test::ComponentTag{})
    SE_FIELD(damage)
    SE_FIELD(type)
    SE_FIELD(tint)
SE_REFLECT_END()


TEST(ReflectMacrosGoldenTest, WeaponIsAutoRegisteredAtStaticInit)
{
    using namespace se_reflect_macros_golden_test;

    // EnsureRegistered<Weapon>()을 이 테스트에서 한 번도 직접 호출하지 않습니다 —
    // SE_REFLECT_BEGIN이 만든 정적 초기화자(_se_reg_kick_*)가 프로그램 시작 시점에
    // 이미 등록을 마쳐놨어야 합니다.
    const se::Optional<const se::TypeInfo&> found = se::TypeRegistry::Get().Find(se::TypeId::Of<Weapon>());
    ASSERT_TRUE(found.HasValue());
    EXPECT_EQ(found->id.Value(), se::TypeId::Of<Weapon>().Value());
}

TEST(ReflectMacrosGoldenTest, WeaponFieldsAreCorrect)
{
    using namespace se_reflect_macros_golden_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Weapon>());
    const se::Optional<const se::StructInfo&> shape = info.AsStruct();
    ASSERT_TRUE(shape.HasValue());
    ASSERT_EQ(shape->fields.Len(), 3u);

    EXPECT_EQ(shape->fields[0].name, "damage");
    EXPECT_EQ(shape->fields[0].type.Value(), se::TypeId::Of<i32>().Value());
    EXPECT_EQ(shape->fields[0].offset, offsetof(Weapon, damage));

    EXPECT_EQ(shape->fields[1].name, "type");
    EXPECT_EQ(shape->fields[1].type.Value(), se::TypeId::Of<EWeaponType>().Value());

    EXPECT_EQ(shape->fields[2].name, "tint");
    EXPECT_EQ(shape->fields[2].type.Value(), se::TypeId::Of<EAutoColor>().Value());
}

TEST(ReflectMacrosGoldenTest, FieldAnnotationsRoundTrip)
{
    using namespace se_reflect_macros_golden_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Weapon>());
    const se::ArrayView<const se::AnnotationRef> annotations = info.AsStruct()->fields[0].annotations;

    ASSERT_EQ(annotations.Len(), 2u);

    const se::AnnotationRef* range_ref = nullptr;
    const se::AnnotationRef* tooltip_ref = nullptr;
    for (const se::AnnotationRef& ref : annotations)
    {
        if (ref.tag.Value() == se::TypeId::Of<RangeTag>().Value()) range_ref = &ref;
        if (ref.tag.Value() == se::TypeId::Of<TooltipTag>().Value()) tooltip_ref = &ref;
    }

    ASSERT_NE(range_ref, nullptr);
    ASSERT_NE(tooltip_ref, nullptr);

    const auto* range = static_cast<const RangeTag*>(range_ref->value);
    EXPECT_FLOAT_EQ(range->min, 0.0f);
    EXPECT_FLOAT_EQ(range->max, 100.0f);

    const auto* tooltip = static_cast<const TooltipTag*>(tooltip_ref->value);
    EXPECT_EQ(tooltip->text, "Damage amount");
}

TEST(ReflectMacrosGoldenTest, TypeLevelAnnotationRoundTrip)
{
    using namespace se_reflect_macros_golden_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<Weapon>());
    ASSERT_EQ(info.annotations.Len(), 1u);
    EXPECT_EQ(info.annotations[0].tag.Value(), se::TypeId::Of<ComponentTag>().Value());
}

TEST(ReflectMacrosGoldenTest, NamedEnumEntriesArePopulated)
{
    using namespace se_reflect_macros_golden_test;

    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<EWeaponType>());
    const se::Optional<const se::EnumInfo&> shape = info.AsEnum();
    ASSERT_TRUE(shape.HasValue());

    ASSERT_EQ(shape->entries.Len(), 3u);
    EXPECT_EQ(shape->entries[0].name, "Sword");
    EXPECT_EQ(shape->entries[0].value, static_cast<i64>(EWeaponType::Sword));
    EXPECT_EQ(shape->entries[1].name, "Bow");
    EXPECT_EQ(shape->entries[2].name, "Staff");
}

TEST(ReflectMacrosGoldenTest, AutoEnumHasEmptyEntriesButKnownUnderlying)
{
    using namespace se_reflect_macros_golden_test;

    // EAutoColor는 SE_DECLARE_REFLECTION도, SE_REFLECT_ENUM_*도 쓴 적이 없습니다 —
    // Weapon::tint 필드로 등장해서 전이적으로, Registrar<T> primary template의
    // is_enum_v 분기가 자동으로 등록했어야 합니다.
    const se::TypeInfo& info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<EAutoColor>());
    const se::Optional<const se::EnumInfo&> shape = info.AsEnum();
    ASSERT_TRUE(shape.HasValue());

    EXPECT_EQ(shape->entries.Len(), 0u);
    EXPECT_EQ(shape->underlying.Value(), se::TypeId::Of<std::underlying_type_t<EAutoColor>>().Value());
}
