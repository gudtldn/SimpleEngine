#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"

#include <cstddef>
#include <variant>


// SE_BASE가 StructInfo::bases를 채우는지, 다중 상속에서 두 번째 base의 오프셋까지 정확한지 검증합니다.
namespace se_reflect_inheritance_test
{
using namespace se;

struct Item
{
    i32 id = 0;
};

struct Durable
{
    f32 durability = 0.0f;
};

struct Weapon : Item
{
    i32 damage = 0;
};

// 두 번째 base(Durable)는 오프셋이 0이 아닙니다.
struct MagicSword : Weapon, Durable
{
    f32 mana = 0.0f;
};

struct Standalone
{
    i32 value = 0;
};

// 자기 등록 블록(SE_REFLECT_BEGIN)이 없어, 파생 타입의 SE_BASE를 통해서만 등록되는 base입니다.
struct BaseRegisteredOnlyThroughDerived
{
    i32 base_value = 0;
};

struct DerivedFromUnregisteredBase : BaseRegisteredOnlyThroughDerived
{
    i32 value = 0;
};

/** 실제 포인터 조정으로 base 오프셋을 구합니다. (기대값 계산용) */
template <typename Base, typename Derived>
usize ActualBaseOffset(const Derived& object)
{
    const auto* base_address = reinterpret_cast<const u8*>(static_cast<const Base*>(&object));
    return static_cast<usize>(base_address - reinterpret_cast<const u8*>(&object));
}

/** 등록된 타입의 StructInfo를 가져옵니다. */
template <typename T>
const StructInfo& StructInfoOf()
{
    const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Of<T>());
    return info.AsStruct().Value();
}
} // namespace se_reflect_inheritance_test

SE_DECLARE_REFLECTION(se_reflect_inheritance_test::Item)
SE_DECLARE_REFLECTION(se_reflect_inheritance_test::Durable)
SE_DECLARE_REFLECTION(se_reflect_inheritance_test::Weapon)
SE_DECLARE_REFLECTION(se_reflect_inheritance_test::MagicSword)
SE_DECLARE_REFLECTION(se_reflect_inheritance_test::Standalone)
SE_DECLARE_REFLECTION(se_reflect_inheritance_test::BaseRegisteredOnlyThroughDerived)
SE_DECLARE_REFLECTION(se_reflect_inheritance_test::DerivedFromUnregisteredBase)

// SE_REFLECT_BEGIN은 정적 초기화 때 타입을 스스로 등록하므로, 이 base의 Fill()은 손으로 작성합니다.
// 그래야 이 타입을 등록하는 경로가 파생 타입의 SE_BASE 하나만 남습니다.
namespace se
{
void Registrar<se_reflect_inheritance_test::BaseRegisteredOnlyThroughDerived>::Fill(TypeInfo& info)
{
    using T = se_reflect_inheritance_test::BaseRegisteredOnlyThroughDerived;
    info.size = sizeof(T);
    info.alignment = alignof(T);
    info.name = TypeNameOf<T>();

    EnsureRegistered<i32>();

    Array<FieldInfo>& fields = TypeRegistry::Get().EmplaceStructStorage(TypeId::Of<T>()).fields;
    fields.Push(FieldInfo{ .name = "base_value", .type = TypeId::Of<i32>(), .offset = offsetof(T, base_value) });

    info.shape = StructInfo{ .fields = fields };
}
} // namespace se

SE_REFLECT_BEGIN(se_reflect_inheritance_test::Item)
    SE_FIELD(id)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_reflect_inheritance_test::Durable)
    SE_FIELD(durability)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_reflect_inheritance_test::Weapon)
    SE_BASE(se_reflect_inheritance_test::Item)
    SE_FIELD(damage)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_reflect_inheritance_test::MagicSword)
    SE_BASE(se_reflect_inheritance_test::Weapon)
    SE_BASE(se_reflect_inheritance_test::Durable)
    SE_FIELD(mana)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_reflect_inheritance_test::Standalone)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_reflect_inheritance_test::DerivedFromUnregisteredBase)
    SE_BASE(se_reflect_inheritance_test::BaseRegisteredOnlyThroughDerived)
    SE_FIELD(value)
SE_REFLECT_END()


TEST(ReflectInheritanceTest, SingleBaseIsRecorded)
{
    using namespace se_reflect_inheritance_test;

    const se::StructInfo& shape = StructInfoOf<Weapon>();
    ASSERT_EQ(shape.bases.Len(), 1u);
    EXPECT_EQ(shape.bases[0].type.Value(), se::TypeId::Of<Item>().Value());

    const Weapon weapon;
    EXPECT_EQ(shape.bases[0].offset, ActualBaseOffset<Item>(weapon));
}

TEST(ReflectInheritanceTest, MultipleBasesKeepDeclarationOrderAndOffsets)
{
    using namespace se_reflect_inheritance_test;

    const se::StructInfo& shape = StructInfoOf<MagicSword>();
    ASSERT_EQ(shape.bases.Len(), 2u);

    EXPECT_EQ(shape.bases[0].type.Value(), se::TypeId::Of<Weapon>().Value());
    EXPECT_EQ(shape.bases[1].type.Value(), se::TypeId::Of<Durable>().Value());

    const MagicSword sword;
    EXPECT_EQ(shape.bases[0].offset, ActualBaseOffset<Weapon>(sword));
    EXPECT_EQ(shape.bases[1].offset, ActualBaseOffset<Durable>(sword));

    // 두 번째 base는 포인터 조정이 일어나므로 오프셋이 0이 아니어야 합니다.
    EXPECT_GT(shape.bases[1].offset, 0u);
}

TEST(ReflectInheritanceTest, TwoLevelHierarchyIsReachableThroughBases)
{
    using namespace se_reflect_inheritance_test;

    // MagicSword -> Weapon -> Item 으로 한 단계씩 따라갈 수 있어야 합니다.
    const se::StructInfo& sword_shape = StructInfoOf<MagicSword>();
    const se::TypeInfo& weapon_info = se::TypeRegistry::Get().FindChecked(sword_shape.bases[0].type);
    const se::Optional<const se::StructInfo&> weapon_shape = weapon_info.AsStruct();
    ASSERT_TRUE(weapon_shape.HasValue());

    ASSERT_EQ(weapon_shape->bases.Len(), 1u);
    EXPECT_EQ(weapon_shape->bases[0].type.Value(), se::TypeId::Of<Item>().Value());
}

TEST(ReflectInheritanceTest, TypeWithoutBaseHasEmptyBases)
{
    using namespace se_reflect_inheritance_test;

    EXPECT_EQ(StructInfoOf<Standalone>().bases.Len(), 0u);
}

TEST(ReflectInheritanceTest, BaseTypesAreRegisteredTransitively)
{
    using namespace se_reflect_inheritance_test;

    // BaseRegisteredOnlyThroughDerived는 스스로 등록하지 않으므로, 파생 타입의 SE_BASE가 전이 등록해야만 찾을 수 있습니다.
    EXPECT_TRUE(se::TypeRegistry::Get().Find(se::TypeId::Of<BaseRegisteredOnlyThroughDerived>()).HasValue());
}
