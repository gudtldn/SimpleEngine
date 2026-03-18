#include "gtest/gtest.h"

#include "SimpleEngine/Graphics/Device/RID.h"
#include "SimpleEngine/Graphics/Device/SlotMap.h"

using namespace se;
using namespace se::graphics;


class RIDTest : public ::testing::Test {};
class SlotMapTest : public ::testing::Test {};


// ============================================================================
// RID Tests
// ============================================================================

TEST_F(RIDTest, DefaultConstructedIsInvalid)
{
    const RID rid{};
    EXPECT_FALSE(rid.IsValid());
    EXPECT_FALSE(static_cast<bool>(rid));
}

TEST_F(RIDTest, ValidRID)
{
    const RID rid{ .index = 0, .generation = 1 };
    EXPECT_TRUE(rid.IsValid());
    EXPECT_TRUE(static_cast<bool>(rid));
}

TEST_F(RIDTest, Equality)
{
    const RID a{ .index = 1, .generation = 2 };
    const RID b{ .index = 1, .generation = 2 };
    const RID c{ .index = 1, .generation = 3 };

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST_F(RIDTest, ToU64Packing)
{
    const RID rid{ .index = 0xDEAD, .generation = 0xBEEF };
    const uint64 packed = rid.ToU64();

    EXPECT_EQ(packed, (static_cast<uint64>(0xBEEF) << 32) | 0xDEAD);
}

TEST_F(RIDTest, HashConsistency)
{
    const RID a{ .index = 42, .generation = 7 };
    const RID b{ .index = 42, .generation = 7 };

    EXPECT_EQ(std::hash<RID>{}(a), std::hash<RID>{}(b));
}


// ============================================================================
// SlotMap Tests
// ============================================================================

TEST_F(SlotMapTest, EmptyOnConstruction)
{
    const SlotMap<int> map;
    EXPECT_TRUE(map.IsEmpty());
    EXPECT_EQ(map.Count(), 0u);
}

TEST_F(SlotMapTest, InsertAndGet)
{
    SlotMap<int> map;
    const RID rid = map.Insert(42);

    EXPECT_TRUE(rid.IsValid());
    EXPECT_EQ(map.Count(), 1u);

    auto value = map.Get(rid);
    EXPECT_TRUE(value.HasValue());
    EXPECT_EQ(*value, 42);
}

TEST_F(SlotMapTest, InsertLvalue)
{
    SlotMap<int> map;
    const int value = 100;
    const RID rid = map.Insert(value);

    auto result = map.Get(rid);
    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(*result, 100);
}

TEST_F(SlotMapTest, GetInvalidRID)
{
    const SlotMap<int> map;
    const RID invalid{};

    EXPECT_FALSE(map.Get(invalid).HasValue());
}

TEST_F(SlotMapTest, GetOutOfBoundsIndex)
{
    const SlotMap<int> map;
    const RID out_of_bounds{ .index = 999, .generation = 1 };

    EXPECT_FALSE(map.Get(out_of_bounds).HasValue());
}

TEST_F(SlotMapTest, Remove)
{
    SlotMap<int> map;
    const RID rid = map.Insert(42);

    EXPECT_TRUE(map.Remove(rid));
    EXPECT_EQ(map.Count(), 0u);
    EXPECT_FALSE(map.Get(rid).HasValue());
}

TEST_F(SlotMapTest, RemoveInvalidRID)
{
    SlotMap<int> map;
    const RID invalid{};

    EXPECT_FALSE(map.Remove(invalid));
}

TEST_F(SlotMapTest, GenerationInvalidatesOldRID)
{
    SlotMap<int> map;
    const RID old_rid = map.Insert(42);

    map.Remove(old_rid);

    // 같은 슬롯을 재사용하지만 세대가 증가합니다.
    const RID new_rid = map.Insert(99);

    // 이전 RID로 접근하면 실패해야 합니다.
    EXPECT_FALSE(map.Get(old_rid).HasValue());
    EXPECT_FALSE(map.IsValidRID(old_rid));

    // 새 RID로 접근하면 성공해야 합니다.
    auto value = map.Get(new_rid);
    EXPECT_TRUE(value.HasValue());
    EXPECT_EQ(*value, 99);
}

TEST_F(SlotMapTest, SlotReuse)
{
    SlotMap<int> map;
    const RID rid1 = map.Insert(1);
    const uint32 first_index = rid1.index;

    map.Remove(rid1);

    const RID rid2 = map.Insert(2);

    // 삭제된 슬롯이 재사용되어야 합니다.
    EXPECT_EQ(rid2.index, first_index);
    // 세대는 증가해야 합니다.
    EXPECT_GT(rid2.generation, rid1.generation);
}

TEST_F(SlotMapTest, MultipleInsertRemove)
{
    SlotMap<int> map;

    Array<RID> rids;
    for (int i = 0; i < 100; ++i)
    {
        rids.Push(map.Insert(i));
    }
    EXPECT_EQ(map.Count(), 100u);

    // 짝수 인덱스 제거
    for (usize i = 0; i < rids.Len(); i += 2)
    {
        map.Remove(rids[i]);
    }
    EXPECT_EQ(map.Count(), 50u);

    // 홀수 인덱스 값 확인
    for (usize i = 1; i < rids.Len(); i += 2)
    {
        auto value = map.Get(rids[i]);
        EXPECT_TRUE(value.HasValue());
        EXPECT_EQ(*value, static_cast<int>(i));
    }

    // 짝수 인덱스는 무효화 확인
    for (usize i = 0; i < rids.Len(); i += 2)
    {
        EXPECT_FALSE(map.Get(rids[i]).HasValue());
    }
}

TEST_F(SlotMapTest, ForEachMutable)
{
    SlotMap<int> map;
    map.Insert(1);
    map.Insert(2);
    map.Insert(3);

    int sum = 0;
    map.ForEach([&sum](RID, int& value)
    {
        sum += value;
        value *= 2;  // 값 수정
    });
    EXPECT_EQ(sum, 6);

    // 수정 확인
    int doubled_sum = 0;
    map.ForEach([&doubled_sum](RID, int& value)
    {
        doubled_sum += value;
    });
    EXPECT_EQ(doubled_sum, 12);
}

TEST_F(SlotMapTest, ForEachConst)
{
    SlotMap<int> map;
    map.Insert(10);
    map.Insert(20);

    const auto& const_map = map;

    int sum = 0;
    const_map.ForEach([&sum](RID, const int& value)
    {
        sum += value;
    });
    EXPECT_EQ(sum, 30);
}

TEST_F(SlotMapTest, ForEachSkipsRemovedSlots)
{
    SlotMap<int> map;
    const RID rid1 = map.Insert(1);
    map.Insert(2);
    map.Insert(3);

    map.Remove(rid1);

    int count = 0;
    map.ForEach([&count](RID, const int&)
    {
        ++count;
    });
    EXPECT_EQ(count, 2);
}

TEST_F(SlotMapTest, GetMutableModifiesValue)
{
    SlotMap<int> map;
    const RID rid = map.Insert(42);

    auto value = map.Get(rid);
    EXPECT_TRUE(value.HasValue());
    *value = 100;

    auto check = map.Get(rid);
    EXPECT_TRUE(check.HasValue());
    EXPECT_EQ(*check, 100);
}

TEST_F(SlotMapTest, ConstGetReturnsConstOptional)
{
    SlotMap<int> map;
    const RID rid = map.Insert(42);

    const auto& const_map = map;
    auto value = const_map.Get(rid);

    EXPECT_TRUE(value.HasValue());
    EXPECT_EQ(*value, 42);

    // const_map.Get(rid) 반환 타입이 Optional<const int&>인지 컴파일 타임 확인
    static_assert(std::is_same_v<decltype(const_map.Get(rid)), Optional<const int&>>);
}

TEST_F(SlotMapTest, Clear)
{
    SlotMap<int> map;
    const RID rid = map.Insert(1);
    map.Insert(2);

    map.Clear();

    EXPECT_TRUE(map.IsEmpty());
    EXPECT_EQ(map.Count(), 0u);
    EXPECT_FALSE(map.Get(rid).HasValue());
}

TEST_F(SlotMapTest, MoveOnlyType)
{
    SlotMap<std::unique_ptr<int>> map;
    const RID rid = map.Insert(std::make_unique<int>(42));

    auto value = map.Get(rid);
    EXPECT_TRUE(value.HasValue());
    EXPECT_NE(*value, nullptr);
    EXPECT_EQ(**value, 42);
}