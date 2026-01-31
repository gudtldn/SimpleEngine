#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"

using namespace se;

// ============================================================================
// StringView Tests
// ============================================================================

class StringViewAPI_Test : public ::testing::Test {};

TEST_F(StringViewAPI_Test, DefaultConstruction)
{
    StringView sv;
    EXPECT_TRUE(sv.IsEmpty());
    EXPECT_EQ(sv.ByteLen(), 0);
    EXPECT_EQ(sv.Data(), nullptr);
}

TEST_F(StringViewAPI_Test, ConstructionFromCString)
{
    StringView sv("hello");
    EXPECT_FALSE(sv.IsEmpty());
    EXPECT_EQ(sv.ByteLen(), 5);
    EXPECT_EQ(sv[0], 'h');
    EXPECT_EQ(sv[4], 'o');
}

TEST_F(StringViewAPI_Test, ConstructionFromPointerAndLength)
{
    const char* str = "hello world";
    StringView sv(str, 5);
    EXPECT_EQ(sv.ByteLen(), 5);
    EXPECT_EQ(sv, "hello");
}

TEST_F(StringViewAPI_Test, ConstructionFromStdStringView)
{
    std::string_view std_sv = "test";
    StringView sv(std_sv);
    EXPECT_EQ(sv.ByteLen(), 4);
    EXPECT_EQ(sv, "test");
}

TEST_F(StringViewAPI_Test, ConstructionFromString)
{
    String str("hello");
    StringView sv(str);
    EXPECT_EQ(sv.ByteLen(), 5);
    EXPECT_EQ(sv, "hello");
}

TEST_F(StringViewAPI_Test, AtSafeAccess)
{
    StringView sv("hello");

    // 유효한 인덱스
    EXPECT_TRUE(sv.At(0).HasValue());
    EXPECT_EQ(sv.At(0).Value(), 'h');
    EXPECT_EQ(sv.At(4).Value(), 'o');

    // 범위 밖 인덱스
    EXPECT_FALSE(sv.At(5).HasValue());
    EXPECT_FALSE(sv.At(100).HasValue());
}

TEST_F(StringViewAPI_Test, FrontAndBack)
{
    StringView sv("hello");

    // 안전한 버전
    EXPECT_TRUE(sv.Front().HasValue());
    EXPECT_EQ(sv.Front().Value(), 'h');
    EXPECT_TRUE(sv.Back().HasValue());
    EXPECT_EQ(sv.Back().Value(), 'o');

    // Checked 버전
    EXPECT_EQ(sv.FrontChecked(), 'h');
    EXPECT_EQ(sv.BackChecked(), 'o');

    // 빈 View
    StringView empty;
    EXPECT_FALSE(empty.Front().HasValue());
    EXPECT_FALSE(empty.Back().HasValue());
}

TEST_F(StringViewAPI_Test, Iterators)
{
    StringView sv("abc");

    std::string result;
    for (char c : sv)
    {
        result += c;
    }
    EXPECT_EQ(result, "abc");
}

TEST_F(StringViewAPI_Test, RemovePrefixAndSuffix)
{
    StringView sv("hello world");

    sv.RemovePrefix(6);
    EXPECT_EQ(sv, "world");

    sv.RemoveSuffix(2);
    EXPECT_EQ(sv, "wor");
}

TEST_F(StringViewAPI_Test, Substr)
{
    StringView sv("hello world");

    EXPECT_EQ(sv.Substr(0, 5), "hello");
    EXPECT_EQ(sv.Substr(6), "world");
    EXPECT_EQ(sv.Substr(6, 100), "world"); // count가 길이보다 큰 경우

    // pos가 범위 밖인 경우
    StringView empty_result = sv.Substr(100);
    EXPECT_TRUE(empty_result.IsEmpty());
}

TEST_F(StringViewAPI_Test, StartsWithAndEndsWith)
{
    StringView sv("hello world");

    EXPECT_TRUE(sv.StartsWith("hello"));
    EXPECT_TRUE(sv.StartsWith(""));
    EXPECT_FALSE(sv.StartsWith("world"));
    EXPECT_FALSE(sv.StartsWith("hello world!"));

    EXPECT_TRUE(sv.EndsWith("world"));
    EXPECT_TRUE(sv.EndsWith(""));
    EXPECT_FALSE(sv.EndsWith("hello"));
    EXPECT_FALSE(sv.EndsWith("hello world!"));
}

TEST_F(StringViewAPI_Test, Contains)
{
    StringView sv("hello world");

    EXPECT_TRUE(sv.Contains("hello"));
    EXPECT_TRUE(sv.Contains("world"));
    EXPECT_TRUE(sv.Contains(" "));
    EXPECT_TRUE(sv.Contains(""));
    EXPECT_FALSE(sv.Contains("xyz"));
}

TEST_F(StringViewAPI_Test, FindStringView)
{
    StringView sv("hello world hello");

    // 찾기 성공
    auto pos1 = sv.Find("hello");
    EXPECT_TRUE(pos1.HasValue());
    EXPECT_EQ(pos1.Value(), 0);

    auto pos2 = sv.Find("world");
    EXPECT_TRUE(pos2.HasValue());
    EXPECT_EQ(pos2.Value(), 6);

    // 시작 위치 지정
    auto pos3 = sv.Find("hello", 1);
    EXPECT_TRUE(pos3.HasValue());
    EXPECT_EQ(pos3.Value(), 12);

    // 찾기 실패
    auto pos4 = sv.Find("xyz");
    EXPECT_FALSE(pos4.HasValue());

    // 빈 문자열 찾기
    auto pos5 = sv.Find("");
    EXPECT_TRUE(pos5.HasValue());
    EXPECT_EQ(pos5.Value(), 0);
}

TEST_F(StringViewAPI_Test, FindChar)
{
    StringView sv("hello");

    auto pos1 = sv.Find('h');
    EXPECT_TRUE(pos1.HasValue());
    EXPECT_EQ(pos1.Value(), 0);

    auto pos2 = sv.Find('l');
    EXPECT_TRUE(pos2.HasValue());
    EXPECT_EQ(pos2.Value(), 2);

    auto pos3 = sv.Find('l', 3);
    EXPECT_TRUE(pos3.HasValue());
    EXPECT_EQ(pos3.Value(), 3);

    auto pos4 = sv.Find('x');
    EXPECT_FALSE(pos4.HasValue());
}

TEST_F(StringViewAPI_Test, RFind)
{
    StringView sv("hello");

    auto pos1 = sv.RFind('l');
    EXPECT_TRUE(pos1.HasValue());
    EXPECT_EQ(pos1.Value(), 3);

    auto pos2 = sv.RFind('h');
    EXPECT_TRUE(pos2.HasValue());
    EXPECT_EQ(pos2.Value(), 0);

    auto pos3 = sv.RFind('x');
    EXPECT_FALSE(pos3.HasValue());

    // 빈 View
    StringView empty;
    EXPECT_FALSE(empty.RFind('a').HasValue());
}

TEST_F(StringViewAPI_Test, Comparison)
{
    StringView sv1("abc");
    StringView sv2("abc");
    StringView sv3("abd");
    StringView sv4("ab");

    EXPECT_EQ(sv1, sv2);
    EXPECT_NE(sv1, sv3);
    EXPECT_NE(sv1, sv4);

    EXPECT_LT(sv1, sv3);
    EXPECT_GT(sv3, sv1);
    EXPECT_GT(sv1, sv4);
}

TEST_F(StringViewAPI_Test, ConversionToStdStringView)
{
    StringView sv("hello");
    std::string_view std_sv = sv;

    EXPECT_EQ(std_sv, "hello");
}

TEST_F(StringViewAPI_Test, ConstexprConstruction)
{
    constexpr StringView sv("hello");
    static_assert(sv.ByteLen() == 5);
    static_assert(sv[0] == 'h');
    static_assert(!sv.IsEmpty());
}

TEST_F(StringViewAPI_Test, ConstexprAt)
{
    constexpr StringView sv("hello");
    static_assert(sv.At(0).HasValue());
    static_assert(sv.At(0).Value() == 'h');
    static_assert(!sv.At(10).HasValue());
}

TEST_F(StringViewAPI_Test, ConstexprFrontBack)
{
    constexpr StringView sv("hello");
    static_assert(sv.Front().HasValue());
    static_assert(sv.Front().Value() == 'h');
    static_assert(sv.Back().HasValue());
    static_assert(sv.Back().Value() == 'o');

    constexpr StringView empty;
    static_assert(!empty.Front().HasValue());
    static_assert(!empty.Back().HasValue());
}

TEST_F(StringViewAPI_Test, ConstexprFind)
{
    constexpr StringView sv("hello world");

    constexpr auto pos1 = sv.Find('o');
    static_assert(pos1.HasValue());
    static_assert(pos1.Value() == 4);

    constexpr auto pos2 = sv.Find('x');
    static_assert(!pos2.HasValue());
}

// ============================================================================
// ArrayView Tests
// ============================================================================

class ArrayViewAPI_Test : public ::testing::Test {};

TEST_F(ArrayViewAPI_Test, DefaultConstruction)
{
    ArrayView<int> view;
    EXPECT_TRUE(view.IsEmpty());
    EXPECT_EQ(view.Len(), 0);
    EXPECT_EQ(view.Data(), nullptr);
}

TEST_F(ArrayViewAPI_Test, ConstructionFromPointerAndSize)
{
    int arr[] = {1, 2, 3, 4, 5};
    ArrayView<int> view(arr, 5);

    EXPECT_FALSE(view.IsEmpty());
    EXPECT_EQ(view.Len(), 5);
    EXPECT_EQ(view[0], 1);
    EXPECT_EQ(view[4], 5);
}

TEST_F(ArrayViewAPI_Test, ConstructionFromTwoPointers)
{
    int arr[] = {1, 2, 3, 4, 5};
    ArrayView<int> view(arr, arr + 3);

    EXPECT_EQ(view.Len(), 3);
    EXPECT_EQ(view[2], 3);
}

TEST_F(ArrayViewAPI_Test, ConstructionFromCArray)
{
    int arr[] = {10, 20, 30};
    ArrayView<int> view(arr);

    EXPECT_EQ(view.Len(), 3);
    EXPECT_EQ(view[0], 10);
}

TEST_F(ArrayViewAPI_Test, ConstructionFromInitializerList)
{
    ArrayView<const int> view = {1, 2, 3, 4};
    EXPECT_EQ(view.Len(), 4);
    EXPECT_EQ(view[0], 1);
    EXPECT_EQ(view[3], 4);
}

TEST_F(ArrayViewAPI_Test, ConstructionFromArray)
{
    Array<int> arr = {1, 2, 3};
    ArrayView<int> view(arr);

    EXPECT_EQ(view.Len(), 3);
    EXPECT_EQ(view[0], 1);

    // const Array
    const Array<int>& const_arr = arr;
    ArrayView<const int> const_view(const_arr);
    EXPECT_EQ(const_view.Len(), 3);
}

TEST_F(ArrayViewAPI_Test, AtSafeAccess)
{
    int arr[] = {10, 20, 30};
    ArrayView<int> view(arr);

    // 유효한 인덱스
    EXPECT_TRUE(view.At(0).HasValue());
    EXPECT_EQ(view.At(0).Value(), 10);
    EXPECT_EQ(view.At(2).Value(), 30);

    // 범위 밖 인덱스
    EXPECT_FALSE(view.At(3).HasValue());
    EXPECT_FALSE(view.At(100).HasValue());
}

TEST_F(ArrayViewAPI_Test, FrontAndBack)
{
    int arr[] = {10, 20, 30};
    ArrayView<int> view(arr);

    // 안전한 버전
    EXPECT_TRUE(view.Front().HasValue());
    EXPECT_EQ(view.Front().Value(), 10);
    EXPECT_TRUE(view.Back().HasValue());
    EXPECT_EQ(view.Back().Value(), 30);

    // Checked 버전
    EXPECT_EQ(view.FrontChecked(), 10);
    EXPECT_EQ(view.BackChecked(), 30);

    // 빈 View
    ArrayView<int> empty;
    EXPECT_FALSE(empty.Front().HasValue());
    EXPECT_FALSE(empty.Back().HasValue());
}

TEST_F(ArrayViewAPI_Test, Modification)
{
    int arr[] = {1, 2, 3};
    ArrayView<int> view(arr);

    view[0] = 100;
    EXPECT_EQ(arr[0], 100);

    view.Front().Value() = 200;
    EXPECT_EQ(arr[0], 200);
}

TEST_F(ArrayViewAPI_Test, Iterators)
{
    int arr[] = {1, 2, 3};
    ArrayView<int> view(arr);

    int sum = 0;
    for (int val : view)
    {
        sum += val;
    }
    EXPECT_EQ(sum, 6);
}

TEST_F(ArrayViewAPI_Test, ReverseIterators)
{
    int arr[] = {1, 2, 3};
    ArrayView<int> view(arr);

    std::vector<int> reversed;
    for (auto it = view.rbegin(); it != view.rend(); ++it)
    {
        reversed.push_back(*it);
    }

    EXPECT_EQ(reversed[0], 3);
    EXPECT_EQ(reversed[1], 2);
    EXPECT_EQ(reversed[2], 1);
}

TEST_F(ArrayViewAPI_Test, First)
{
    int arr[] = {1, 2, 3, 4, 5};
    ArrayView<int> view(arr);

    auto first3 = view.First(3);
    EXPECT_EQ(first3.Len(), 3);
    EXPECT_EQ(first3[0], 1);
    EXPECT_EQ(first3[2], 3);

    // count가 길이보다 큰 경우
    auto first10 = view.First(10);
    EXPECT_EQ(first10.Len(), 5);
}

TEST_F(ArrayViewAPI_Test, Last)
{
    int arr[] = {1, 2, 3, 4, 5};
    ArrayView<int> view(arr);

    auto last3 = view.Last(3);
    EXPECT_EQ(last3.Len(), 3);
    EXPECT_EQ(last3[0], 3);
    EXPECT_EQ(last3[2], 5);

    // count가 길이보다 큰 경우
    auto last10 = view.Last(10);
    EXPECT_EQ(last10.Len(), 5);
}

TEST_F(ArrayViewAPI_Test, Subview)
{
    int arr[] = {1, 2, 3, 4, 5};
    ArrayView<int> view(arr);

    auto sub1 = view.Subview(1, 3);
    EXPECT_EQ(sub1.Len(), 3);
    EXPECT_EQ(sub1[0], 2);
    EXPECT_EQ(sub1[2], 4);

    auto sub2 = view.Subview(3);
    EXPECT_EQ(sub2.Len(), 2);
    EXPECT_EQ(sub2[0], 4);

    // offset이 범위 밖인 경우
    auto empty = view.Subview(100);
    EXPECT_TRUE(empty.IsEmpty());
}

TEST_F(ArrayViewAPI_Test, Comparison)
{
    int arr1[] = {1, 2, 3};
    int arr2[] = {1, 2, 3};
    int arr3[] = {1, 2, 4};
    int arr4[] = {1, 2};

    ArrayView<int> view1(arr1);
    ArrayView<int> view2(arr2);
    ArrayView<int> view3(arr3);
    ArrayView<int> view4(arr4);

    EXPECT_EQ(view1, view2);
    EXPECT_NE(view1, view3);
    EXPECT_NE(view1, view4);

    EXPECT_LT(view1, view3);
    EXPECT_GT(view3, view1);
    EXPECT_GT(view1, view4);
}

TEST_F(ArrayViewAPI_Test, NonConstToConstConversion)
{
    int arr[] = {1, 2, 3};
    ArrayView<int> mutable_view(arr);
    ArrayView<const int> const_view = mutable_view;

    EXPECT_EQ(const_view.Len(), 3);
    EXPECT_EQ(const_view[0], 1);
}

TEST_F(ArrayViewAPI_Test, ByteSize)
{
    int arr[] = {1, 2, 3};
    ArrayView<int> view(arr);

    EXPECT_EQ(view.ByteSize(), 3 * sizeof(int));
}

TEST_F(ArrayViewAPI_Test, ConstexprConstruction)
{
    static constexpr int arr[] = {1, 2, 3};
    constexpr ArrayView<const int> view(arr);

    static_assert(view.Len() == 3);
    static_assert(view[0] == 1);
    static_assert(!view.IsEmpty());
}

TEST_F(ArrayViewAPI_Test, ConstexprAt)
{
    static constexpr int arr[] = {10, 20, 30};
    constexpr ArrayView<const int> view(arr);

    static_assert(view.At(0).HasValue());
    static_assert(view.At(0).Value() == 10);
    static_assert(!view.At(5).HasValue());
}

TEST_F(ArrayViewAPI_Test, ConstexprFrontBack)
{
    static constexpr int arr[] = {10, 20, 30};
    constexpr ArrayView<const int> view(arr);

    static_assert(view.Front().HasValue());
    static_assert(view.Front().Value() == 10);
    static_assert(view.Back().HasValue());
    static_assert(view.Back().Value() == 30);

    constexpr ArrayView<const int> empty;
    static_assert(!empty.Front().HasValue());
    static_assert(!empty.Back().HasValue());
}

TEST_F(ArrayViewAPI_Test, DeductionGuides)
{
    int arr[] = {1, 2, 3};

    // 포인터 + 크기
    ArrayView view1(arr, 3u);
    EXPECT_EQ(view1.Len(), 3);

    // 두 포인터
    ArrayView view2(arr, arr + 2);
    EXPECT_EQ(view2.Len(), 2);

    // C 배열
    ArrayView view3(arr);
    EXPECT_EQ(view3.Len(), 3);

    // initializer_list
    ArrayView view4 = {1, 2, 3, 4};
    EXPECT_EQ(view4.Len(), 4);
}
