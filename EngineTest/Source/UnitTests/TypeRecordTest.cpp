#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/TypeRecordRegistry.h"

#include <algorithm>


// TypeRecordRegistry가 조상 체인을 오프셋까지 정확히 평탄화하는지 검증합니다.
namespace se_type_record_test
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

// 가상 상속 없는 다이아몬드 구조 — DiamondBase가 두 서브오브젝트로 중복됩니다.
struct DiamondBase
{
    i32 v = 0;
};

struct Left : DiamondBase
{
    i32 l = 0;
};

struct Right : DiamondBase
{
    i32 r = 0;
};

struct Diamond : Left, Right
{
    i32 d = 0;
};

/** 실제 포인터 조정으로 base 오프셋을 구합니다. (기대값 계산용) */
template <typename Base, typename Derived>
usize ActualBaseOffset(const Derived& object)
{
    const auto* base_address = reinterpret_cast<const u8*>(static_cast<const Base*>(&object));
    return static_cast<usize>(base_address - reinterpret_cast<const u8*>(&object));
}

/** 타입 T를 등록하고 그 TypeRecord를 가져옵니다. */
template <typename T>
const TypeRecord& RecordOf()
{
    EnsureRegistered<T>();
    return TypeRecordRegistry::Get().Find(TypeId::Of<T>()).Value();
}
} // namespace se_type_record_test

SE_DECLARE_REFLECTION(se_type_record_test::Item)
SE_DECLARE_REFLECTION(se_type_record_test::Durable)
SE_DECLARE_REFLECTION(se_type_record_test::Weapon)
SE_DECLARE_REFLECTION(se_type_record_test::MagicSword)
SE_DECLARE_REFLECTION(se_type_record_test::Standalone)
SE_DECLARE_REFLECTION(se_type_record_test::DiamondBase)
SE_DECLARE_REFLECTION(se_type_record_test::Left)
SE_DECLARE_REFLECTION(se_type_record_test::Right)
SE_DECLARE_REFLECTION(se_type_record_test::Diamond)

SE_REFLECT_BEGIN(se_type_record_test::Item)
    SE_FIELD(id)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::Durable)
    SE_FIELD(durability)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::Weapon)
    SE_BASE(se_type_record_test::Item)
    SE_FIELD(damage)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::MagicSword)
    SE_BASE(se_type_record_test::Weapon)
    SE_BASE(se_type_record_test::Durable)
    SE_FIELD(mana)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::Standalone)
    SE_FIELD(value)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::DiamondBase)
    SE_FIELD(v)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::Left)
    SE_BASE(se_type_record_test::DiamondBase)
    SE_FIELD(l)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::Right)
    SE_BASE(se_type_record_test::DiamondBase)
    SE_FIELD(r)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_type_record_test::Diamond)
    SE_BASE(se_type_record_test::Left)
    SE_BASE(se_type_record_test::Right)
    SE_FIELD(d)
SE_REFLECT_END()


TEST(TypeRecordTest, SelfEntryHasZeroOffset)
{
    using namespace se_type_record_test;

    const se::TypeRecord& record = RecordOf<MagicSword>();
    ASSERT_FALSE(record.all_bases.IsEmpty());
    EXPECT_EQ(record.all_bases[0].type.Value(), se::TypeId::Of<MagicSword>().Value());
    EXPECT_EQ(record.all_bases[0].offset, 0u);
}

TEST(TypeRecordTest, AllOffsetsMatchActualPointerAdjustment)
{
    using namespace se_type_record_test;

    const se::TypeRecord& record = RecordOf<MagicSword>();
    const MagicSword sword;

    // 개수를 먼저 고정하지 않으면 조상을 전부 빠뜨려도 아래 루프가 공허하게 통과합니다.
    ASSERT_EQ(record.all_bases.Len(), 4u); // MagicSword, Weapon, Item, Durable

    for (const se::CastEntry& entry : record.all_bases)
    {
        if (entry.type.Value() == se::TypeId::Of<MagicSword>().Value())
        {
            EXPECT_EQ(entry.offset, ActualBaseOffset<MagicSword>(sword));
        }
        else if (entry.type.Value() == se::TypeId::Of<Weapon>().Value())
        {
            EXPECT_EQ(entry.offset, ActualBaseOffset<Weapon>(sword));
        }
        else if (entry.type.Value() == se::TypeId::Of<Item>().Value())
        {
            EXPECT_EQ(entry.offset, ActualBaseOffset<Item>(sword));
        }
        else if (entry.type.Value() == se::TypeId::Of<Durable>().Value())
        {
            EXPECT_EQ(entry.offset, ActualBaseOffset<Durable>(sword));
        }
        else
        {
            FAIL() << "예상치 못한 조상 타입입니다.";
        }
    }
}

TEST(TypeRecordTest, TransitiveAncestorIsIncluded)
{
    using namespace se_type_record_test;

    const se::TypeRecord& record = RecordOf<MagicSword>();
    const bool has_item = std::ranges::any_of(record.all_bases, [](const se::CastEntry& entry)
    {
        return entry.type.Value() == se::TypeId::Of<Item>().Value();
    });
    EXPECT_TRUE(has_item);
}

TEST(TypeRecordTest, StandaloneHasSingleEntry)
{
    using namespace se_type_record_test;

    const se::TypeRecord& record = RecordOf<Standalone>();
    ASSERT_EQ(record.all_bases.Len(), 1u);
    EXPECT_EQ(record.all_bases[0].type.Value(), se::TypeId::Of<Standalone>().Value());
    EXPECT_EQ(record.all_bases[0].offset, 0u);
}

TEST(TypeRecordTest, NonStructTypesHaveSingleEntry)
{
    using namespace se_type_record_test;

    const se::TypeRecord& i32_record = RecordOf<i32>();
    ASSERT_EQ(i32_record.all_bases.Len(), 1u);
    EXPECT_EQ(i32_record.all_bases[0].offset, 0u);

    const se::TypeRecord& array_record = RecordOf<se::Array<i32>>();
    ASSERT_EQ(array_record.all_bases.Len(), 1u);
    EXPECT_EQ(array_record.all_bases[0].offset, 0u);
}

TEST(TypeRecordTest, DiamondBaseAppearsTwiceWithDifferentOffsets)
{
    using namespace se_type_record_test;

    const se::TypeRecord& record = RecordOf<Diamond>();
    ASSERT_EQ(record.all_bases.Len(), 5u); // Diamond, Left, DiamondBase, Right, DiamondBase

    se::Array<usize> diamond_base_offsets;
    for (const se::CastEntry& entry : record.all_bases)
    {
        if (entry.type.Value() == se::TypeId::Of<DiamondBase>().Value())
        {
            diamond_base_offsets.Push(entry.offset);
        }
    }

    ASSERT_EQ(diamond_base_offsets.Len(), 2u);
    EXPECT_NE(diamond_base_offsets[0], diamond_base_offsets[1]);
}
