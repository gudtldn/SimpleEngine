#include "gtest/gtest.h"
#include "SimpleEngine/Core/Container/FlatMap.h"
#include "SimpleEngine/Core/Container/FlatSet.h"
#include "SimpleEngine/Core/Container/String.h"

using namespace se;

class FlatSetAPI_Test : public ::testing::Test {};
class FlatMapAPI_Test : public ::testing::Test {};

// --- FlatSet Tests ---

TEST_F(FlatSetAPI_Test, DefaultConstruction)
{
    FlatSet<int> set;
    EXPECT_EQ(set.Len(), 0);
    EXPECT_TRUE(set.IsEmpty());
}

TEST_F(FlatSetAPI_Test, InitializerListConstruction)
{
    FlatSet<int> set = { 3, 1, 4, 1, 5, 9, 2, 6, 5 };
    // 중복 제거 및 정렬 확인: {1, 2, 3, 4, 5, 6, 9}
    EXPECT_EQ(set.Len(), 7);

    int expected[] = { 1, 2, 3, 4, 5, 6, 9 };
    int i = 0;
    for (int val : set)
    {
        EXPECT_EQ(val, expected[i++]);
    }
}

TEST_F(FlatSetAPI_Test, InsertAndContains)
{
    FlatSet<int> set;
    EXPECT_TRUE(set.Insert(10));
    EXPECT_TRUE(set.Insert(20));
    EXPECT_FALSE(set.Insert(10)); // 중복 삽입

    EXPECT_EQ(set.Len(), 2);
    EXPECT_TRUE(set.Contains(10));
    EXPECT_TRUE(set.Contains(20));
    EXPECT_FALSE(set.Contains(30));
}

TEST_F(FlatSetAPI_Test, Remove)
{
    FlatSet<int> set = { 10, 20, 30 };
    EXPECT_TRUE(set.Remove(20));
    EXPECT_FALSE(set.Remove(20)); // 이미 삭제됨
    EXPECT_EQ(set.Len(), 2);
    EXPECT_FALSE(set.Contains(20));
}

TEST_F(FlatSetAPI_Test, Clear)
{
    FlatSet<int> set = { 1, 2, 3 };
    set.Clear();
    EXPECT_EQ(set.Len(), 0);
    EXPECT_TRUE(set.IsEmpty());
}

// --- FlatMap Tests ---

TEST_F(FlatMapAPI_Test, DefaultConstruction)
{
    FlatMap<int, String> map;
    EXPECT_EQ(map.Len(), 0);
    EXPECT_TRUE(map.IsEmpty());
}

TEST_F(FlatMapAPI_Test, InitializerListConstruction)
{
    FlatMap<int, String> map = {
        { 3, "Three" },
        { 1, "One" },
        { 2, "Two" },
        { 1, "Duplicate" } // 중복 키
    };

    // 정렬 확인 (키 기준)
    EXPECT_EQ(map.Len(), 3);
    EXPECT_TRUE(map.Contains(1));
    EXPECT_TRUE(map.Contains(2));
    EXPECT_TRUE(map.Contains(3));
}

TEST_F(FlatMapAPI_Test, InsertAndAccess)
{
    FlatMap<int, String> map;
    map.Insert(1, "One");
    map.Emplace(2, "Two");

    EXPECT_EQ(map[1], "One");
    EXPECT_EQ(map[2], "Two");

    map.Insert(3, "Three");
    EXPECT_EQ(map.Len(), 3);
    EXPECT_EQ(map.FindChecked(3), "Three");
}

TEST_F(FlatMapAPI_Test, Find)
{
    FlatMap<int, String> map = { { 10, "Ten" } };

    auto result = map.Find(10);
    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(*result, "Ten");

    auto fail = map.Find(20);
    EXPECT_FALSE(fail.HasValue());
}

TEST_F(FlatMapAPI_Test, Remove)
{
    FlatMap<int, String> map = { { 1, "One" }, { 2, "Two" } };
    EXPECT_TRUE(map.Remove(1));
    EXPECT_FALSE(map.Remove(1));
    EXPECT_EQ(map.Len(), 1);
    EXPECT_FALSE(map.Contains(1));
}

TEST_F(FlatMapAPI_Test, EntryAPI)
{
    FlatMap<int, int> map;

    // Vacant Entry
    auto entry = map.Entry(10);
    EXPECT_TRUE(entry.IsVacant());
    entry.OrInsert(100);
    EXPECT_EQ(map[10], 100);

    // Occupied Entry
    auto entry2 = map.Entry(10);
    EXPECT_TRUE(entry2.IsOccupied());
    entry2.AndModify([](int& val) { val += 50; });
    EXPECT_EQ(map[10], 150);
}

TEST_F(FlatMapAPI_Test, KeysAndValues)
{
    FlatMap<int, String> map = { { 1, "A" }, { 2, "B" }, { 3, "C" } };

    auto keys = map.Keys();
    EXPECT_EQ(keys.Len(), 3);
    EXPECT_EQ(keys[0], 1);
    EXPECT_EQ(keys[2], 3);

    auto values = map.Values();
    EXPECT_EQ(values.Len(), 3);
    EXPECT_EQ(values[0], "A");
    EXPECT_EQ(values[2], "C");
}

TEST_F(FlatMapAPI_Test, LowerUpperBound)
{
    FlatMap<int, String> map = { { 10, "Ten" }, { 30, "Thirty" }, { 50, "Fifty" } };

    auto lb = map.LowerBoundEntry(20); // 30 기대
    EXPECT_TRUE(lb.HasValue());
    EXPECT_EQ(lb->first, 30);

    auto lb2 = map.LowerBoundEntry(30); // 30 기대
    EXPECT_TRUE(lb2.HasValue());
    EXPECT_EQ(lb2->first, 30);

    auto ub = map.UpperBoundEntry(30); // 50 기대
    EXPECT_TRUE(ub.HasValue());
    EXPECT_EQ(ub->first, 50);
}
