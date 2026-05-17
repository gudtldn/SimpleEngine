#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/Container/FlatMap.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/IterChain.h"
#include "SimpleEngine/Core/Container/Map.h"

#include <tuple>
#include <vector>

class IterChainTest : public ::testing::Test {};

using namespace se;

// -------------------------------------------------------------------------
// Array 기반 기본 체인
// -------------------------------------------------------------------------

TEST_F(IterChainTest, MapAndFilter)
{
    Array<int> arr = { 1, 2, 3, 4, 5 };

    const auto result = arr
        .Iter()
        .Filter([](int x) { return x % 2 == 0; })
        .Map([](int x) { return x * 10; })
        .Collect<Array<int>>();

    ASSERT_EQ(result.Len(), 2u);
    EXPECT_EQ(result[0], 20);
    EXPECT_EQ(result[1], 40);
}

TEST_F(IterChainTest, CollectIntoArray)
{
    Array<int> arr = { 3, 1, 4, 1, 5 };
    const auto result = arr.Iter().Collect<Array<int>>();

    ASSERT_EQ(result.Len(), 5u);
    EXPECT_EQ(result[0], 3);
    EXPECT_EQ(result[4], 5);
}

TEST_F(IterChainTest, CollectIntoStdVector)
{
    Array<int> arr = { 1, 2, 3 };
    const auto result = arr.Iter().Collect<std::vector<int>>();

    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 1);
}

TEST_F(IterChainTest, FlatMap)
{
    Array<Array<int>> arr = { Array<int>{ 1, 2 }, Array<int>{ 3, 4 } };

    const auto result = arr
        .Iter()
        .FlatMap([](const Array<int>& inner) -> const Array<int>& { return inner; })
        .Collect<Array<int>>();

    ASSERT_EQ(result.Len(), 4u);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[3], 4);
}

TEST_F(IterChainTest, TakeAndSkip)
{
    Array<int> arr = { 0, 1, 2, 3, 4 };

    const auto taken = arr.Iter().Take(3).Collect<Array<int>>();
    ASSERT_EQ(taken.Len(), 3u);
    EXPECT_EQ(taken[2], 2);

    const auto skipped = arr.Iter().Skip(2).Collect<Array<int>>();
    ASSERT_EQ(skipped.Len(), 3u);
    EXPECT_EQ(skipped[0], 2);
}

TEST_F(IterChainTest, Enumerate)
{
    Array<int> arr = { 10, 20, 30 };

    usize sum_idx = 0;
    int  sum_val  = 0;
    arr.Iter().Enumerate().ForEach([&](auto pair)
    {
        auto [idx, val] = pair;
        sum_idx += static_cast<usize>(idx);
        sum_val += val;
    });

    EXPECT_EQ(sum_idx, 3u);  // 0+1+2
    EXPECT_EQ(sum_val, 60);  // 10+20+30
}

TEST_F(IterChainTest, Reverse)
{
    Array<int> arr = { 1, 2, 3 };
    const auto result = arr.Iter().Reverse().Collect<Array<int>>();

    ASSERT_EQ(result.Len(), 3u);
    EXPECT_EQ(result[0], 3);
    EXPECT_EQ(result[2], 1);
}

TEST_F(IterChainTest, Zip)
{
    Array<int> a = { 1, 2, 3 };
    Array<int> b = { 10, 20, 30 };

    int sum = 0;
    a.Iter().Zip(b.Iter()).ForEach([&](auto pair)
    {
        auto [x, y] = pair;
        sum += x + y;
    });

    EXPECT_EQ(sum, 66);  // (1+10)+(2+20)+(3+30)
}

TEST_F(IterChainTest, Fold)
{
    Array<int> arr = { 1, 2, 3, 4, 5 };
    const int result = arr.Iter().Fold(0, [](int acc, int x) { return acc + x; });
    EXPECT_EQ(result, 15);
}

TEST_F(IterChainTest, AnyAll)
{
    Array<int> arr = { 2, 4, 6, 8 };
    EXPECT_TRUE(arr.Iter().All([](int x) { return x % 2 == 0; }));
    EXPECT_FALSE(arr.Iter().Any([](int x) { return x % 2 != 0; }));

    Array<int> mixed = { 1, 2, 3 };
    EXPECT_TRUE(mixed.Iter().Any([](int x) { return x == 2; }));
    EXPECT_FALSE(mixed.Iter().All([](int x) { return x > 2; }));
}

TEST_F(IterChainTest, Count)
{
    Array<int> arr = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(arr.Iter().Filter([](int x) { return x > 2; }).Count(), 3u);
}

TEST_F(IterChainTest, Find)
{
    Array<int> arr = { 1, 2, 3, 4, 5 };
    const Optional<int> found = arr.Iter().Find([](int x) { return x > 3; });
    EXPECT_TRUE(found.HasValue());
    EXPECT_EQ(found.Value(), 4);

    const Optional<int> not_found = arr.Iter().Find([](int x) { return x > 10; });
    EXPECT_FALSE(not_found.HasValue());
}

TEST_F(IterChainTest, SumProduct)
{
    Array<int> arr = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(arr.Iter().Sum(), 15);
    EXPECT_EQ(arr.Iter().Product(), 120);
}

TEST_F(IterChainTest, MinMax)
{
    Array<int> arr = { 3, 1, 4, 1, 5, 9, 2, 6 };
    EXPECT_EQ(arr.Iter().Min().Value(), 1);
    EXPECT_EQ(arr.Iter().Max().Value(), 9);

    Array<int> empty_arr;
    EXPECT_FALSE(empty_arr.Iter().Min().HasValue());
    EXPECT_FALSE(empty_arr.Iter().Max().HasValue());
}

TEST_F(IterChainTest, RvalueOwnership)
{
    auto make_arr = []
    {
        Array<int> a;
        a.Push(1);
        a.Push(2);
        a.Push(3);
        return a;
    };

    // rvalue에서 Iter()를 호출하면 owning_view로 데이터를 소유합니다.
    const auto result = make_arr().Iter().Map([](int x) { return x * 2; }).Collect<Array<int>>();
    ASSERT_EQ(result.Len(), 3u);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[2], 6);
}

// -------------------------------------------------------------------------
// Map 기반 체인
// -------------------------------------------------------------------------

TEST_F(IterChainTest, MapContainerIter)
{
    Map<int, int> m;
    m.Insert(1, 10);
    m.Insert(2, 20);
    m.Insert(3, 30);

    // Map의 Iter()는 pair<const int, int>를 순회합니다.
    const int sum_keys = m.Iter()
        .Map([](const auto& kv) { return kv.first; })
        .Fold(0, [](int acc, int k) { return acc + k; });

    EXPECT_EQ(sum_keys, 6);
}

TEST_F(IterChainTest, HashMapContainerIter)
{
    HashMap<int, int> hm;
    hm.Insert(1, 100);
    hm.Insert(2, 200);

    const bool all_positive = hm.Iter().All([](const auto& kv) { return kv.second > 0; });
    EXPECT_TRUE(all_positive);
}
