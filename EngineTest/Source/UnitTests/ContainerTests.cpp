#include "gtest/gtest.h"

// ReSharper disable once CppUnusedIncludeDirective
#include <iostream>
#include <vector>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Container/PriorityQueue.h"
#include "SimpleEngine/Core/Container/Set.h"
#include "SimpleEngine/Core/Container/String.h"

class FixedArrayAPI_Test : public ::testing::Test {};
class ArrayAPI_Test : public ::testing::Test {};
class StringAPI_Test : public ::testing::Test {};
class DequeAPI_Test : public ::testing::Test {};
class HashMapAPI_Test : public ::testing::Test {};
class MapAPI_Test : public ::testing::Test {};
class HashSetAPI_Test : public ::testing::Test {};
class SetAPI_Test : public ::testing::Test {};
class PriorityQueueAPI_Test : public ::testing::Test {};

// Using the namespace where containers are defined
using namespace se;


TEST_F(FixedArrayAPI_Test, DefaultConstructionAndCapacity)
{
    FixedArray<int, 5> arr;
    EXPECT_EQ(arr.Len(), 5);
    EXPECT_FALSE(arr.IsEmpty());
}

TEST_F(FixedArrayAPI_Test, ElementAccessWithAt)
{
    FixedArray<int, 3> arr;
    *arr.At(0) = 10;
    *arr.At(1) = 20;
    *arr.At(2) = 30;

    EXPECT_TRUE(arr.At(0).HasValue());
    EXPECT_EQ(*arr.At(0), 10);

    EXPECT_TRUE(arr.At(1).HasValue());
    EXPECT_EQ(*arr.At(1), 20);

    EXPECT_TRUE(arr.At(2).HasValue());
    EXPECT_EQ(*arr.At(2), 30);

    EXPECT_FALSE(arr.At(3).HasValue());
    EXPECT_FALSE(arr.At(static_cast<usize>(-1)).HasValue());
}

TEST_F(FixedArrayAPI_Test, CopyConstructionAndAssignment)
{
    FixedArray<int, 3> arr1;
    *arr1.At(0) = 1;
    *arr1.At(1) = 2;
    *arr1.At(2) = 3;

    FixedArray<int, 3> arr2 = arr1; // 복사 생성
    EXPECT_EQ(*arr2.At(0), 1);
    EXPECT_EQ(*arr2.At(1), 2);
    EXPECT_EQ(*arr2.At(2), 3);

    FixedArray<int, 3> arr3;
    arr3 = arr1; // 복사 할당
    EXPECT_EQ(*arr3.At(0), 1);
    EXPECT_EQ(*arr3.At(1), 2);
    EXPECT_EQ(*arr3.At(2), 3);
}

TEST_F(FixedArrayAPI_Test, RangeBasedForLoop)
{
    FixedArray<int, 4> arr;
    *arr.At(0) = 1;
    *arr.At(1) = 2;
    *arr.At(2) = 3;
    *arr.At(3) = 4;

    int sum = 0;
    for (int val : arr)
    {
        sum += val;
    }
    EXPECT_EQ(sum, 10);
}

TEST_F(FixedArrayAPI_Test, ConstexprOperations)
{
    // FixedArray의 생성 및 기본 메서드가 constexpr로 동작하는지 확인
    constexpr FixedArray<int, 3> CONST_ARR{}; // {}를 사용하여 집계 초기화
    static_assert(CONST_ARR.Len() == 3, "Len() should be constexpr");
    static_assert(!CONST_ARR.IsEmpty(), "IsEmpty() should be constexpr");

    // At() 메서드는 Optional을 반환하므로, Optional이 constexpr이 아닐 경우 직접적인 constexpr 테스트는 어렵습니다.
    // 하지만 At()의 반환 타입이 포인터라면 constexpr 테스트가 가능합니다.
    // 현재 Optional<T&>를 반환하므로, Optional이 constexpr이 될 때까지는 주석 처리합니다.
    /*
    // constexpr FixedArray<int, 2> const_arr_with_values = {10, 20}; // FixedArray는 집계 초기화가 아니므로 직접 초기화 불가
    // static_assert(const_arr_with_values.At(0).HasValue(), "At() should be constexpr when Optional is constexpr");
    // static_assert(*const_arr_with_values.At(0) == 10, "At() should return correct value");
    */

    // Data() 메서드는 포인터를 반환하므로 constexpr 테스트가 가능합니다.
    constexpr FixedArray<int, 1> data_arr{};
    static_assert(data_arr.Data() != nullptr, "Data() should be constexpr and return a valid pointer");
}

TEST_F(ArrayAPI_Test, DefaultConstruction)
{
    Array<int> arr;
    EXPECT_EQ(arr.Len(), 0);
    EXPECT_TRUE(arr.IsEmpty());
    EXPECT_EQ(arr.Capacity(), 0);
}

TEST_F(ArrayAPI_Test, Uninitialized)
{
    // This is only safe for trivially default constructible types
    Array<int> arr = Array<int>::Uninitialized(5);
    EXPECT_EQ(arr.Len(), 5);
    EXPECT_GE(arr.Capacity(), 5);

    // The values are uninitialized, so we just write to them
    arr[0] = 1;
    arr[4] = 5;
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[4], 5);
}

TEST_F(ArrayAPI_Test, ConstructionWithSize)
{
    Array<int> arr(5);
    EXPECT_EQ(arr.Len(), 5);
    EXPECT_FALSE(arr.IsEmpty());
    EXPECT_GE(arr.Capacity(), 5);
    // Elements are default-initialized
    EXPECT_EQ(arr[0], 0);
}

TEST_F(ArrayAPI_Test, ConstructionWithSizeAndValue)
{
    Array<int> arr(3, 10);
    EXPECT_EQ(arr.Len(), 3);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 10);
    EXPECT_EQ(arr[2], 10);
}

TEST_F(ArrayAPI_Test, InitializerListConstruction)
{
    Array<int> arr = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(arr.Len(), 5);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[4], 5);
}

TEST_F(ArrayAPI_Test, ConstructionFromRange)
{
    std::vector<int> vec = { 1, 2, 3, 4, 5 };
    Array<int> arr_from_vec(vec.begin(), vec.end());
    EXPECT_EQ(arr_from_vec.Len(), 5);
    EXPECT_EQ(arr_from_vec[0], 1);
    EXPECT_EQ(arr_from_vec[4], 5);

    Array<int> arr_from_range = Array<int>::FromRange(vec);
    EXPECT_EQ(arr_from_range.Len(), 5);
    EXPECT_EQ(arr_from_range[0], 1);
    EXPECT_EQ(arr_from_range[4], 5);
}

TEST_F(ArrayAPI_Test, PushAndPop)
{
    Array<int> arr;
    arr.Push(10);
    arr.Push(20);
    EXPECT_EQ(arr.Len(), 2);
    EXPECT_EQ(arr[1], 20);

    auto popped = arr.Pop();
    EXPECT_TRUE(popped.HasValue());
    EXPECT_EQ(*popped, 20);
    EXPECT_EQ(arr.Len(), 1);

    popped = arr.Pop();
    EXPECT_TRUE(popped.HasValue());
    EXPECT_EQ(*popped, 10);
    EXPECT_TRUE(arr.IsEmpty());

    popped = arr.Pop();
    EXPECT_FALSE(popped.HasValue());
}

TEST_F(ArrayAPI_Test, ElementAccess)
{
    Array<int> arr = { 10, 20, 30 };
    EXPECT_EQ(*arr.At(1), 20);
    EXPECT_FALSE(arr.At(3).HasValue());

    EXPECT_EQ(*arr.Front(), 10);
    EXPECT_EQ(*arr.Back(), 30);

    arr[0] = 15;
    EXPECT_EQ(arr[0], 15);

    const Array<int> const_arr = { 1, 2 };
    EXPECT_EQ(*const_arr.At(0), 1);
    EXPECT_EQ(*const_arr.Front(), 1);
    EXPECT_EQ(*const_arr.Back(), 2);
    EXPECT_EQ(const_arr[1], 2);
}

TEST_F(ArrayAPI_Test, Data)
{
    Array<int> arr = { 1, 2, 3 };
    int* data_ptr = arr.Data();
    EXPECT_EQ(data_ptr[0], 1);
    EXPECT_EQ(data_ptr[1], 2);
    EXPECT_EQ(data_ptr[2], 3);

    const Array<int> const_arr = { 4, 5, 6 };
    const int* const_data_ptr = const_arr.Data();
    EXPECT_EQ(const_data_ptr[0], 4);
    EXPECT_EQ(const_data_ptr[1], 5);
    EXPECT_EQ(const_data_ptr[2], 6);
}

TEST_F(ArrayAPI_Test, EmptyArrayAccess)
{
    Array<int> arr;
    EXPECT_FALSE(arr.Front().HasValue());
    EXPECT_FALSE(arr.Back().HasValue());
    EXPECT_FALSE(arr.At(0).HasValue());
}

TEST_F(ArrayAPI_Test, CapacityAndReserving)
{
    Array<int> arr;
    arr.Reserve(10);
    EXPECT_GE(arr.Capacity(), 10);
    EXPECT_EQ(arr.Len(), 0);

    arr.Push(1);
    arr.ShrinkToFit();
    EXPECT_EQ(arr.Capacity(), 1);
    EXPECT_EQ(arr.Len(), 1);
}

TEST_F(ArrayAPI_Test, Resize)
{
    Array<int> arr = { 1, 2, 3 };
    arr.Resize(5);
    EXPECT_EQ(arr.Len(), 5);
    EXPECT_EQ(arr[2], 3);
    // New elements are default-initialized
    EXPECT_EQ(arr[3], 0);
    EXPECT_EQ(arr[4], 0);

    arr.Resize(2);
    EXPECT_EQ(arr.Len(), 2);
    EXPECT_EQ(arr[1], 2);

    // Test resize with value
    arr.Resize(4, 99);
    EXPECT_EQ(arr.Len(), 4);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 99);
    EXPECT_EQ(arr[3], 99);
}

TEST_F(ArrayAPI_Test, ResizeUninitialize)
{
    Array<int> arr = { 1, 2, 3 };
    arr.ResizeUninitialized(5);
    EXPECT_EQ(arr.Len(), 5);
    EXPECT_EQ(arr[2], 3);
    // New elements are default-initialized
    EXPECT_TRUE(arr.At(3).HasValue());
    EXPECT_TRUE(arr.At(4).HasValue());

    arr.ResizeUninitialized(2);
    EXPECT_EQ(arr.Len(), 2);
    EXPECT_EQ(arr[1], 2);
}

TEST_F(ArrayAPI_Test, Truncate)
{
    Array<int> arr = { 1, 2, 3, 4, 5 };
    arr.Truncate(3);
    EXPECT_EQ(arr.Len(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);

    // Truncate to a larger or equal size should do nothing
    arr.Truncate(5);
    EXPECT_EQ(arr.Len(), 3);
}

TEST_F(ArrayAPI_Test, Clear)
{
    Array<int> arr = { 1, 2, 3 };
    arr.Clear();
    EXPECT_EQ(arr.Len(), 0);
    EXPECT_TRUE(arr.IsEmpty());
    EXPECT_GE(arr.Capacity(), 3); // Capacity is not changed
}

TEST_F(ArrayAPI_Test, InsertAndRemove)
{
    Array<int> arr = { 10, 20, 30 };
    arr.Insert(1, 15); // {10, 15, 20, 30}
    EXPECT_EQ(arr.Len(), 4);
    EXPECT_EQ(arr[1], 15);
    EXPECT_EQ(arr[2], 20);

    arr.RemoveAt(2); // {10, 15, 30}
    EXPECT_EQ(arr.Len(), 3);
    EXPECT_EQ(arr[2], 30);
}

TEST_F(ArrayAPI_Test, InsertRange)
{
    Array<int> arr = { 10, 40 };
    std::vector<int> to_insert = { 20, 30 };

    // Test with iterators
    arr.Insert(1, to_insert.begin(), to_insert.end());
    EXPECT_EQ(arr.Len(), 4);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 20);
    EXPECT_EQ(arr[2], 30);
    EXPECT_EQ(arr[3], 40);

    // Test with range
    Array<int> arr2 = { 10, 40 };
    arr2.InsertRange(1, to_insert);
    EXPECT_EQ(arr2.Len(), 4);
    EXPECT_EQ(arr2[0], 10);
    EXPECT_EQ(arr2[1], 20);
    EXPECT_EQ(arr2[2], 30);
    EXPECT_EQ(arr2[3], 40);
}

TEST_F(ArrayAPI_Test, Remove)
{
    Array<int> arr = { 1, 2, 1, 1, 2, 3, 1 };
    usize removed_count = arr.Remove(1);
    EXPECT_EQ(removed_count, 4);
    EXPECT_EQ(arr.Len(), 3);
    EXPECT_EQ(arr[0], 2);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}


TEST_F(ArrayAPI_Test, RemoveRange)
{
    Array<int> arr = { 10, 20, 30, 40, 50, 60 };
    arr.RemoveRange(2, 3); // Remove 30, 40, 50
    EXPECT_EQ(arr.Len(), 3);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 20);
    EXPECT_EQ(arr[2], 60);
}

TEST_F(ArrayAPI_Test, RemoveIf)
{
    Array<int> arr = { 1, 2, 3, 4, 5, 6 };
    usize removed_count = arr.RemoveIf([](int val) { return val % 2 == 0; }); // Remove even numbers
    EXPECT_EQ(removed_count, 3);
    EXPECT_EQ(arr.Len(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 3);
    EXPECT_EQ(arr[2], 5);
}

TEST_F(ArrayAPI_Test, RemoveAtSwap)
{
    Array<int> arr = { 10, 20, 30, 40 };
    arr.RemoveAtSwap(1); // swap 20 with 40, then pop
    EXPECT_EQ(arr.Len(), 3);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 40); // 20 was swapped with 40
    EXPECT_EQ(arr[2], 30);

    // arr.RemoveAtSwap(5); // out of bounds

    // Test removing the last element
    Array<int> arr2 = { 1, 2, 3 };
    arr2.RemoveAtSwap(2);
    EXPECT_EQ(arr2.Len(), 2);
    EXPECT_EQ(arr2[0], 1);
    EXPECT_EQ(arr2[1], 2);
}

TEST_F(ArrayAPI_Test, ContainsAndFind)
{
    Array<int> arr = { 10, 20, 30, 20 };
    EXPECT_TRUE(arr.Contains(20));
    EXPECT_FALSE(arr.Contains(50));

    auto index = arr.Find(20);
    EXPECT_TRUE(index.HasValue());
    EXPECT_EQ(*index, 1); // Finds first occurrence

    index = arr.Find(50);
    EXPECT_FALSE(index.HasValue());
}

TEST_F(ArrayAPI_Test, Push)
{
    Array<int> arr1 = { 1, 2 };
    Array<int> arr2 = { 3, 4 };
    arr1.PushRange(arr2);
    EXPECT_EQ(arr1.Len(), 4);
    EXPECT_EQ(arr1[2], 3);
    EXPECT_EQ(arr1[3], 4);

    std::vector<int> vec = { 5, 6 };
    arr1.PushRange(vec);
    EXPECT_EQ(arr1.Len(), 6);
    EXPECT_EQ(arr1[4], 5);
    EXPECT_EQ(arr1[5], 6);
}

TEST_F(ArrayAPI_Test, Emplace)
{
    struct TestStruct
    {
        int x;
        f64 y;

        TestStruct(int x, f64 y)
            : x(x)
            , y(y)
        {
        }
    };
    Array<TestStruct> arr;
    TestStruct& val = arr.Emplace(1, 3.14);
    EXPECT_EQ(arr.Len(), 1);
    EXPECT_EQ(arr[0].x, 1);
    EXPECT_EQ(arr[0].y, 3.14);
    EXPECT_EQ(val.x, 1);
    EXPECT_EQ(val.y, 3.14);
}

TEST_F(ArrayAPI_Test, RangeBasedForLoop)
{
    Array<int> arr = { 1, 2, 3, 4 };
    int sum = 0;
    for (const int val : arr)
    {
        sum += val;
    }
    EXPECT_EQ(sum, 10);
}

TEST_F(ArrayAPI_Test, CopyAndMoveSemantics)
{
    Array<int> arr1 = { 1, 2, 3 };
    Array<int> arr2 = arr1; // Copy constructor
    EXPECT_EQ(arr1.Len(), 3);
    EXPECT_EQ(arr2.Len(), 3);
    EXPECT_EQ(arr2[1], 2);

    Array<int> arr3;
    arr3 = arr1; // Copy assignment
    EXPECT_EQ(arr3.Len(), 3);
    EXPECT_EQ(arr3[1], 2);

    Array<int> arr4 = std::move(arr1); // Move constructor
    EXPECT_EQ(arr4.Len(), 3);
    EXPECT_EQ(arr4[1], 2);
    // arr1 is in a valid but unspecified state, but should be empty
    EXPECT_TRUE(arr1.IsEmpty());

    Array<int> arr5;
    arr5 = std::move(arr3); // Move assignment
    EXPECT_EQ(arr5.Len(), 3);
    EXPECT_EQ(arr5[1], 2);
    EXPECT_TRUE(arr3.IsEmpty());
}

TEST_F(StringAPI_Test, Construction)
{
    String s1;
    EXPECT_TRUE(s1.IsEmpty());
    EXPECT_EQ(s1.ByteLen(), 0);
    EXPECT_EQ(s1.CodePointLen(), 0);

    String s2("hello");
    EXPECT_EQ(s2.ByteLen(), 5);
    EXPECT_EQ(s2.CodePointLen(), 5);
    EXPECT_EQ(s2, "hello");

    // UTF-8 string: "안녕하세요" (5 characters, 15 bytes)
    String s3("안녕하세요");
    EXPECT_EQ(s3.ByteLen(), 15);
    EXPECT_EQ(s3.CodePointLen(), 5);
    EXPECT_EQ(s3, "안녕하세요");

    String s4(U'😊', 3); // 3 smiling faces
    EXPECT_EQ(s4, "😊😊😊");
    EXPECT_EQ(s4.ByteLen(), 12); // 4 bytes per emoji
    EXPECT_EQ(s4.CodePointLen(), 3);

    String s5(std::string_view("world"));
    EXPECT_EQ(s5, "world");

    String s6 = s5; // Copy construction
    EXPECT_EQ(s6, "world");

    String s7 = std::move(s6); // Move construction
    EXPECT_EQ(s7, "world");
    EXPECT_TRUE(s6.IsEmpty()); // Moved-from state
}

TEST_F(StringAPI_Test, Assignment)
{
    String s;
    s = "test";
    EXPECT_EQ(s, "test");
    s = std::string_view("another");
    EXPECT_EQ(s, "another");
    s = U'X';
    EXPECT_EQ(s, "X");
    EXPECT_EQ(s.ByteLen(), 1);
    EXPECT_EQ(s.CodePointLen(), 1);
}

TEST_F(StringAPI_Test, PushAndConcatenation)
{
    String s1("Hello");
    s1.Append(" World");
    EXPECT_EQ(s1, "Hello World");

    String s2 = s1 + "!";
    EXPECT_EQ(s2, "Hello World!");

    s2 += U'😊';
    EXPECT_EQ(s2, "Hello World!😊");

    String s3 = "Prefix: " + s2;
    EXPECT_TRUE(s3.StartsWith("Prefix: "));
}

TEST_F(StringAPI_Test, FindAndContains)
{
    String s("Hello World, Hello Universe");
    EXPECT_TRUE(s.Contains("World"));
    EXPECT_FALSE(s.Contains("Galaxy"));

    EXPECT_TRUE(s.StartsWith("Hello"));
    EXPECT_TRUE(s.EndsWith("Universe"));

    auto find_res = s.Find("Hello");
    EXPECT_TRUE(find_res.HasValue());
    EXPECT_EQ(*find_res, 0);

    find_res = s.Find("Hello", 1);
    EXPECT_TRUE(find_res.HasValue());
    EXPECT_EQ(*find_res, 13);

    auto rfind_res = s.FindLast("Hello");
    EXPECT_TRUE(rfind_res.HasValue());
    EXPECT_EQ(*rfind_res, 13);
}

TEST_F(StringAPI_Test, Substrings)
{
    String s("0123456789");
    String sub1 = s.Substring(2, 5);
    EXPECT_EQ(sub1, "23456");

    String sub2 = s.Substring(5);
    EXPECT_EQ(sub2, "56789");

    StringView view1 = s.SubstringView(2, 5);
    EXPECT_EQ(view1, "23456");
}

TEST_F(StringAPI_Test, InsertAndRemove)
{
    String s("Hello Universe");
    s.Insert(6, "Beautiful ");
    EXPECT_EQ(s, "Hello Beautiful Universe");

    // Remove "Beautiful " (10 bytes)
    s.RemoveRange(6, 10);
    EXPECT_EQ(s, "Hello Universe");
}

TEST_F(StringAPI_Test, PushAndPop)
{
    String s("abc");
    s.Push(U'😊');
    EXPECT_EQ(s, "abc😊");

    Optional<char32_t> popped = s.Pop();
    EXPECT_TRUE(popped.HasValue());
    EXPECT_EQ(*popped, U'😊');
    EXPECT_EQ(s, "abc");

    s.Pop();
    s.Pop();
    s.Pop();
    EXPECT_TRUE(s.IsEmpty());

    popped = s.Pop();
    EXPECT_FALSE(popped.HasValue());
}

TEST_F(StringAPI_Test, CaseConversion)
{
    String s("Hello World");
    String upper = s.ToUpper();
    String lower = s.ToLower();
    EXPECT_EQ(upper, "HELLO WORLD");
    EXPECT_EQ(lower, "hello world");

    // Turkish 'i' test
    String turkish_i("Iıİi");
    String turkish_upper = turkish_i.ToUpper("tr_TR");
    String turkish_lower = turkish_i.ToLower("tr_TR");
    EXPECT_EQ(turkish_upper, "IIİİ");
    EXPECT_EQ(turkish_lower, "ııii");
}

TEST_F(StringAPI_Test, CodePointsAndBytesView)
{
    String s("a😊b"); // a(1) + emoji(4) + b(1) = 6 bytes
    EXPECT_EQ(s.ByteLen(), 6);
    EXPECT_EQ(s.CodePointLen(), 3);

    int cp_count = 0;
    for ([[maybe_unused]] char32_t cp : s.CodePoints())
    {
        cp_count++;
    }
    EXPECT_EQ(cp_count, 3);

    int byte_count = 0;
    for ([[maybe_unused]] char byte : s.Bytes())
    {
        byte_count++;
    }
    EXPECT_EQ(byte_count, 6);
}

TEST_F(StringAPI_Test, Comparison)
{
    String s1("abc");
    String s2("abd");
    String s3("abc");

    EXPECT_EQ(s1, s3);
    EXPECT_NE(s1, s2);
    EXPECT_LT(s1, s2);
    EXPECT_GT(s2, s1);
    EXPECT_LE(s1, s3);
}

TEST_F(StringAPI_Test, StaticFormat)
{
    auto s = String::Format("The number is {} and the string is '{}'.", 42, "test");
    EXPECT_EQ(s, "The number is 42 and the string is 'test'.");
}


TEST_F(DequeAPI_Test, Construction)
{
    // Default
    Deque<int> d1;
    EXPECT_TRUE(d1.IsEmpty());
    EXPECT_EQ(d1.Len(), 0);

    // With size
    Deque<int> d2(5);
    EXPECT_EQ(d2.Len(), 5);
    EXPECT_EQ(d2[0], 0);

    // With size and value
    Deque<int> d3(3, 100);
    EXPECT_EQ(d3.Len(), 3);
    EXPECT_EQ(d3[0], 100);
    EXPECT_EQ(d3[1], 100);
    EXPECT_EQ(d3[2], 100);

    // Initializer list
    Deque<int> d4 = { 1, 2, 3 };
    EXPECT_EQ(d4.Len(), 3);
    EXPECT_EQ(d4[1], 2);

    // From iterators
    std::vector<int> vec = { 4, 5, 6 };
    Deque<int> d5(vec.begin(), vec.end());
    EXPECT_EQ(d5.Len(), 3);
    EXPECT_EQ(d5[1], 5);

    // From range
    Deque<int> d6 = Deque<int>::FromRange(vec);
    EXPECT_EQ(d6.Len(), 3);
    EXPECT_EQ(d6[1], 5);
}

TEST_F(DequeAPI_Test, PushAndPop)
{
    Deque<int> d;
    d.PushBack(10);
    d.PushFront(20); // {20, 10}
    d.PushBack(30);  // {20, 10, 30}
    EXPECT_EQ(d.Len(), 3);
    EXPECT_EQ(d[0], 20);
    EXPECT_EQ(d[1], 10);
    EXPECT_EQ(d[2], 30);

    auto pop_back = d.PopBack();
    EXPECT_TRUE(pop_back.HasValue());
    EXPECT_EQ(*pop_back, 30);
    EXPECT_EQ(d.Len(), 2);

    auto pop_front = d.PopFront();
    EXPECT_TRUE(pop_front.HasValue());
    EXPECT_EQ(*pop_front, 20);
    EXPECT_EQ(d.Len(), 1);

    d.PopBack();
    EXPECT_TRUE(d.IsEmpty());

    EXPECT_FALSE(d.PopFront().HasValue());
    EXPECT_FALSE(d.PopBack().HasValue());
}

TEST_F(DequeAPI_Test, Emplace)
{
    struct TestStruct
    {
        int x;
        f64 y;
        TestStruct(int x, f64 y) : x(x), y(y) {}
    };

    Deque<TestStruct> d;
    d.EmplaceBack(1, 1.1);
    d.EmplaceFront(2, 2.2);
    EXPECT_EQ(d.Len(), 2);
    EXPECT_EQ(d[0].x, 2);
    EXPECT_EQ(d[1].x, 1);
}

TEST_F(DequeAPI_Test, ElementAccess)
{
    Deque<int> d = { 10, 20, 30 };
    EXPECT_EQ(d[1], 20);

    *d.At(0) = 15;
    EXPECT_EQ(*d.At(0), 15);
    EXPECT_FALSE(d.At(3).HasValue());

    EXPECT_EQ(*d.Front(), 15);
    EXPECT_EQ(*d.Back(), 30);

    const Deque<int> cd = d;
    EXPECT_EQ(cd[1], 20);
    EXPECT_EQ(*cd.At(0), 15);
    EXPECT_EQ(*cd.Front(), 15);
    EXPECT_EQ(*cd.Back(), 30);
}

TEST_F(DequeAPI_Test, ResizeAndClear)
{
    Deque<int> d = { 1, 2, 3, 4, 5 };
    d.Resize(3);
    EXPECT_EQ(d.Len(), 3);
    EXPECT_EQ(d[2], 3);

    d.Resize(5, 100);
    EXPECT_EQ(d.Len(), 5);
    EXPECT_EQ(d[3], 100);
    EXPECT_EQ(d[4], 100);

    d.Clear();
    EXPECT_TRUE(d.IsEmpty());
    EXPECT_EQ(d.Len(), 0);
}

TEST_F(DequeAPI_Test, ShrinkToFit)
{
    Deque<int> d;
    d.PushBack(1);
    d.PushBack(2);
    d.PushBack(3);
    // NOTE: std::deque doesn't have capacity(), so we can't directly test if it shrank.
    // We just call it to ensure it compiles and doesn't crash.
    d.ShrinkToFit();
    EXPECT_EQ(d.Len(), 3);
}

TEST_F(DequeAPI_Test, Insert)
{
    Deque<int> d = { 10, 50 };
    d.Insert(1, 20); // {10, 20, 50}
    EXPECT_EQ(d.Len(), 3);
    EXPECT_EQ(d[1], 20);

    std::vector<int> vec = { 30, 40 };
    d.InsertRange(2, vec); // {10, 20, 30, 40, 50}
    EXPECT_EQ(d.Len(), 5);
    EXPECT_EQ(d[2], 30);
    EXPECT_EQ(d[3], 40);
}

TEST_F(DequeAPI_Test, Remove)
{
    Deque<int> d = { 10, 20, 30, 40, 50 };
    d.RemoveAt(1); // remove 20 -> {10, 30, 40, 50}
    EXPECT_EQ(d.Len(), 4);
    EXPECT_EQ(d[1], 30);

    Deque<int> d2 = { 1, 2, 1, 3, 1 };
    auto removed_count = d2.Remove(1);
    EXPECT_EQ(removed_count, 3);
    EXPECT_EQ(d2.Len(), 2);
    EXPECT_EQ(d2[0], 2);
    EXPECT_EQ(d2[1], 3);

    Deque<int> d3 = { 1, 2, 3, 4, 5, 6 };
    removed_count = d3.RemoveIf([](int val) { return val % 2 != 0; }); // remove odd
    EXPECT_EQ(removed_count, 3);
    EXPECT_EQ(d3.Len(), 3);
    EXPECT_EQ(d3[0], 2);
    EXPECT_EQ(d3[1], 4);
    EXPECT_EQ(d3[2], 6);
}

TEST_F(DequeAPI_Test, Contains)
{
    Deque<int> d = { 10, 20, 30 };
    EXPECT_TRUE(d.Contains(20));
    EXPECT_FALSE(d.Contains(99));
}

TEST_F(DequeAPI_Test, Swap)
{
    Deque<int> d1 = { 1, 2, 3 };
    Deque<int> d2 = { 4, 5 };
    d1.Swap(d2);
    EXPECT_EQ(d1.Len(), 2);
    EXPECT_EQ(d1[0], 4);
    EXPECT_EQ(d2.Len(), 3);
    EXPECT_EQ(d2[0], 1);
}

TEST_F(DequeAPI_Test, RangeBasedForLoop)
{
    Deque<int> d = { 1, 2, 3, 4 };
    int sum = 0;
    for (const int val : d)
    {
        sum += val;
    }
    EXPECT_EQ(sum, 10);
}

TEST_F(DequeAPI_Test, CopyAndMoveSemantics)
{
    Deque<int> d1 = { 1, 2, 3 };
    Deque<int> d2 = d1; // Copy constructor
    EXPECT_EQ(d1.Len(), 3);
    EXPECT_EQ(d2.Len(), 3);
    EXPECT_EQ(d2[1], 2);

    Deque<int> d3;
    d3 = d1; // Copy assignment
    EXPECT_EQ(d3.Len(), 3);
    EXPECT_EQ(d3[1], 2);

    Deque<int> d4 = std::move(d1); // Move constructor
    EXPECT_EQ(d4.Len(), 3);
    EXPECT_EQ(d4[1], 2);
    EXPECT_TRUE(d1.IsEmpty()); // Moved-from state for std::deque is empty

    Deque<int> d5;
    d5 = std::move(d3); // Move assignment
    EXPECT_EQ(d5.Len(), 3);
    EXPECT_EQ(d5[1], 2);
    EXPECT_TRUE(d3.IsEmpty());
}

TEST_F(HashMapAPI_Test, Construction)
{
    HashMap<String, int> map1;
    EXPECT_TRUE(map1.IsEmpty());
    EXPECT_EQ(map1.Len(), 0);

    HashMap<String, int> map2 = { { "one", 1 }, { "two", 2 } };
    EXPECT_EQ(map2.Len(), 2);
    EXPECT_EQ(*map2.Find("one"), 1);

    std::vector<std::pair<const String, int>> vec = { { "three", 3 }, { "four", 4 } };
    HashMap<String, int> map3(vec.begin(), vec.end());
    EXPECT_EQ(map3.Len(), 2);
    EXPECT_EQ(*map3.Find("three"), 3);

    auto map4 = HashMap<String, int>::FromRange(vec);
    EXPECT_EQ(map4.Len(), 2);
    EXPECT_EQ(*map4.Find("four"), 4);
}

TEST_F(HashMapAPI_Test, AccessAndModification)
{
    HashMap<String, int> map;
    map["one"] = 1;
    EXPECT_EQ(map.Len(), 1);
    EXPECT_EQ(map["one"], 1);

    map["one"] = 11;
    EXPECT_EQ(map["one"], 11);

    EXPECT_TRUE(map.Contains("one"));
    EXPECT_FALSE(map.Contains("two"));

    auto find_res = map.Find("one");
    EXPECT_TRUE(find_res.HasValue());
    EXPECT_EQ(*find_res, 11);

    auto find_res_const = static_cast<const decltype(map)&>(map).Find("one");
    EXPECT_TRUE(find_res_const.HasValue());
    EXPECT_EQ(*find_res_const, 11);

    EXPECT_FALSE(map.Find("two").HasValue());
}

TEST_F(HashMapAPI_Test, RemoveAndClear)
{
    HashMap<String, int> map = { { "one", 1 }, { "two", 2 }, { "three", 3 } };
    EXPECT_TRUE(map.Remove("two"));
    EXPECT_EQ(map.Len(), 2);
    EXPECT_FALSE(map.Contains("two"));
    EXPECT_FALSE(map.Remove("four"));

    map.Clear();
    EXPECT_TRUE(map.IsEmpty());
    EXPECT_EQ(map.Len(), 0);
    EXPECT_GT(map.Capacity(), 0); // Clear does not affect capacity
}

TEST_F(HashMapAPI_Test, CapacityAndReserve)
{
    HashMap<String, int> map;
    // EXPECT_EQ(map.Capacity(), 0);
    map.Reserve(10);
    EXPECT_GE(map.Capacity(), 10);
    map["one"] = 1;
    EXPECT_EQ(map.Len(), 1);
}

TEST_F(HashMapAPI_Test, EntryAPI)
{
    HashMap<String, int> map;
    map["existing"] = 10;

    // Insert new entry
    auto entry1 = map.Entry("new");
    EXPECT_FALSE(entry1.IsOccupied());
    int& val1 = entry1.OrInsert(20);
    EXPECT_EQ(val1, 20);
    EXPECT_EQ(*map.Find("new"), 20);

    // Access existing entry
    auto entry2 = map.Entry("existing");
    EXPECT_TRUE(entry2.IsOccupied());
    int& val2 = entry2.OrInsert(0); // Should not insert
    EXPECT_EQ(val2, 10);
    *entry2.GetValue() = 11;
    EXPECT_EQ(*map.Find("existing"), 11);

    // Modify with function
    auto entry3 = map.Entry("new");
    entry3.AndModify([](int& v) { v *= 2; });
    EXPECT_EQ(*map.Find("new"), 40);
}

TEST_F(HashMapAPI_Test, GetKeysAndGetValues)
{
    HashMap<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
    Array<String> keys = map.Keys();
    Array<int> values = map.Values();

    EXPECT_EQ(keys.Len(), 3);
    EXPECT_EQ(values.Len(), 3);

    // The order is not guaranteed in HashMap, so we EXPECT_TRUE for presence
    EXPECT_TRUE(keys.Contains("a"));
    EXPECT_TRUE(keys.Contains("b"));
    EXPECT_TRUE(keys.Contains("c"));

    EXPECT_TRUE(values.Contains(1));
    EXPECT_TRUE(values.Contains(2));
    EXPECT_TRUE(values.Contains(3));
}

TEST_F(HashMapAPI_Test, RemoveIf)
{
    HashMap<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 } };
    usize removed_count = map.RemoveIf([]([[maybe_unused]] const String& key, const int& val)
    {
        return val % 2 == 0; // Remove even values
    });
    EXPECT_EQ(removed_count, 2);
    EXPECT_EQ(map.Len(), 2);
    EXPECT_TRUE(map.Contains("a"));
    EXPECT_FALSE(map.Contains("b"));
    EXPECT_TRUE(map.Contains("c"));
    EXPECT_FALSE(map.Contains("d"));
}

TEST_F(HashMapAPI_Test, RangeBasedForLoop)
{
    HashMap<String, int> map = { { "one", 1 }, { "two", 2 } };
    int sum = 0;
    for (auto& value : map | std::views::values)
    {
        sum += value;
    }
    EXPECT_EQ(sum, 3);
}

TEST_F(HashMapAPI_Test, Emplace)
{
    HashMap<String, String> map;

    // Emplace a new element
    String& val1 = map.Emplace("key1", "value1");
    EXPECT_EQ(map.Len(), 1);
    EXPECT_EQ(val1, "value1");
    EXPECT_EQ(*map.Find("key1"), "value1");

    // Emplace on existing key
    String& val2 = map.Emplace("key1", "value2_should_not_be_inserted");
    EXPECT_EQ(map.Len(), 1);
    EXPECT_EQ(val2, "value1"); // Should return ref to existing value
    EXPECT_EQ(*map.Find("key1"), "value1");

    // Modify through returned reference
    val2 = "modified_value";
    EXPECT_EQ(*map.Find("key1"), "modified_value");
}

TEST_F(MapAPI_Test, Construction)
{
    Map<String, int> map1;
    EXPECT_TRUE(map1.IsEmpty());
    EXPECT_EQ(map1.Len(), 0);

    Map<String, int> map2 = { { "one", 1 }, { "two", 2 } };
    EXPECT_EQ(map2.Len(), 2);
    EXPECT_EQ(*map2.Find("one"), 1);

    std::vector<std::pair<const String, int>> vec = { { "three", 3 }, { "four", 4 } };
    Map<String, int> map3(vec.begin(), vec.end());
    EXPECT_EQ(map3.Len(), 2);
    EXPECT_EQ(*map3.Find("three"), 3);

    auto map4 = Map<String, int>::FromRange(vec);
    EXPECT_EQ(map4.Len(), 2);
    EXPECT_EQ(*map4.Find("four"), 4);
}

TEST_F(MapAPI_Test, AccessAndModification)
{
    Map<String, int> map;
    map["one"] = 1;
    EXPECT_EQ(map.Len(), 1);
    EXPECT_EQ(map["one"], 1);

    map["one"] = 11;
    EXPECT_EQ(map["one"], 11);

    EXPECT_TRUE(map.Contains("one"));
    EXPECT_FALSE(map.Contains("two"));

    auto find_res = map.Find("one");
    EXPECT_TRUE(find_res.HasValue());
    EXPECT_EQ(*find_res, 11);

    auto find_res_const = static_cast<const decltype(map)&>(map).Find("one");
    EXPECT_TRUE(find_res_const.HasValue());
    EXPECT_EQ(*find_res_const, 11);

    EXPECT_FALSE(map.Find("two").HasValue());
}

TEST_F(MapAPI_Test, RemoveAndClear)
{
    Map<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
    EXPECT_TRUE(map.Remove("b"));
    EXPECT_EQ(map.Len(), 2);
    EXPECT_FALSE(map.Contains("b"));
    EXPECT_FALSE(map.Remove("d"));

    map.Clear();
    EXPECT_TRUE(map.IsEmpty());
    EXPECT_EQ(map.Len(), 0);
}

TEST_F(MapAPI_Test, EntryAPI)
{
    Map<String, int> map;
    map["existing"] = 10;

    // Insert new entry
    auto entry1 = map.Entry("new");
    EXPECT_FALSE(entry1.IsOccupied());
    int& val1 = entry1.OrInsert(20);
    EXPECT_EQ(val1, 20);
    EXPECT_EQ(*map.Find("new"), 20);

    // Access existing entry
    auto entry2 = map.Entry("existing");
    EXPECT_TRUE(entry2.IsOccupied());
    int& val2 = entry2.OrInsert(0); // Should not insert
    EXPECT_EQ(val2, 10);
    *entry2.GetValue() = 11;
    EXPECT_EQ(*map.Find("existing"), 11);

    // Modify with function
    auto entry3 = map.Entry("new");
    entry3.AndModify([](int& v) { v *= 2; });
    EXPECT_EQ(*map.Find("new"), 40);
}

TEST_F(MapAPI_Test, GetKeysAndGetValues)
{
    Map<String, int> map = { { "c", 3 }, { "a", 1 }, { "b", 2 } };
    Array<String> keys = map.Keys();
    Array<int> values = map.Values();

    EXPECT_EQ(keys.Len(), 3);
    EXPECT_EQ(values.Len(), 3);

    // Map guarantees order
    EXPECT_EQ(keys[0], "a");
    EXPECT_EQ(keys[1], "b");
    EXPECT_EQ(keys[2], "c");

    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);
}

TEST_F(MapAPI_Test, RemoveIf)
{
    Map<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 } };
    usize removed_count = map.RemoveIf([]([[maybe_unused]] const String& key, const int& val)
    {
        return val % 2 == 0; // Remove even values
    });
    EXPECT_EQ(removed_count, 2);
    EXPECT_EQ(map.Len(), 2);
    EXPECT_TRUE(map.Contains("a"));
    EXPECT_FALSE(map.Contains("b"));
    EXPECT_TRUE(map.Contains("c"));
    EXPECT_FALSE(map.Contains("d"));
}

TEST_F(MapAPI_Test, RangeBasedForLoop)
{
    Map<String, int> map = { { "one", 1 }, { "two", 2 } };
    int sum = 0;
    String key_concat;
    // Order should be "one", "two" if String comparison is as expected
    for (auto& [key, value] : map)
    {
        sum += value;
        key_concat.Append(key);
    }
    EXPECT_EQ(sum, 3);
    EXPECT_EQ(key_concat, "onetwo");
}

TEST_F(MapAPI_Test, Emplace)
{
    Map<String, String> map;

    // Emplace a new element
    String& val1 = map.Emplace("key1", "value1");
    EXPECT_EQ(map.Len(), 1);
    EXPECT_EQ(val1, "value1");
    EXPECT_EQ(*map.Find("key1"), "value1");

    // Emplace on existing key
    String& val2 = map.Emplace("key1", "value2_should_not_be_inserted");
    EXPECT_EQ(map.Len(), 1);
    EXPECT_EQ(val2, "value1"); // Should return ref to existing value
    EXPECT_EQ(*map.Find("key1"), "value1");

    // Modify through returned reference
    val2 = "modified_value";
    EXPECT_EQ(*map.Find("key1"), "modified_value");
}

TEST_F(MapAPI_Test, First_Last_Bounds)
{
    Map<int, String> map = { { 10, "ten" }, { 20, "twenty" }, { 30, "thirty" } };

    EXPECT_TRUE(map.First().HasValue());
    EXPECT_EQ(map.First()->first, 10);
    EXPECT_EQ(map.First()->second, "ten");

    EXPECT_TRUE(map.Last().HasValue());
    EXPECT_EQ(map.Last()->first, 30);
    EXPECT_EQ(map.Last()->second, "thirty");

    auto lower = map.LowerBoundEntry(20);
    EXPECT_TRUE(lower.HasValue());
    EXPECT_EQ(lower->first, 20);

    auto lower2 = map.LowerBoundEntry(21);
    EXPECT_TRUE(lower2.HasValue());
    EXPECT_EQ(lower2->first, 30);

    auto upper = map.UpperBoundEntry(20);
    EXPECT_TRUE(upper.HasValue());
    EXPECT_EQ(upper->first, 30);

    auto upper2 = map.UpperBoundEntry(30);
    EXPECT_FALSE(upper2.HasValue());

    Map<int, String> empty_map;
    EXPECT_FALSE(empty_map.First().HasValue());
    EXPECT_FALSE(empty_map.Last().HasValue());
    EXPECT_FALSE(empty_map.LowerBoundEntry(1).HasValue());
    EXPECT_FALSE(empty_map.UpperBoundEntry(1).HasValue());
}

TEST_F(HashSetAPI_Test, DefaultConstruction)
{
    HashSet<int> set;
    EXPECT_TRUE(set.IsEmpty());
    EXPECT_EQ(set.Len(), 0);
}

TEST_F(HashSetAPI_Test, ConstructionWithCapacity)
{
    HashSet<int> set(10);
    EXPECT_TRUE(set.IsEmpty());
    EXPECT_EQ(set.Len(), 0);
    EXPECT_GE(set.Capacity(), 10);
}

TEST_F(HashSetAPI_Test, InitializerListConstruction)
{
    HashSet<int> set = { 1, 2, 3, 2, 1 };
    EXPECT_EQ(set.Len(), 3);
    EXPECT_TRUE(set.Contains(1));
    EXPECT_TRUE(set.Contains(2));
    EXPECT_TRUE(set.Contains(3));
    EXPECT_FALSE(set.Contains(4));
}

TEST_F(HashSetAPI_Test, ConstructionFromIteratorsAndRange)
{
    std::vector<int> vec = { 10, 20, 30, 20 };
    HashSet<int> set_from_iter(vec.begin(), vec.end());
    EXPECT_EQ(set_from_iter.Len(), 3);
    EXPECT_TRUE(set_from_iter.Contains(10));
    EXPECT_TRUE(set_from_iter.Contains(20));
    EXPECT_TRUE(set_from_iter.Contains(30));

    HashSet<int> set_from_range = HashSet<int>::FromRange(vec);
    EXPECT_EQ(set_from_range.Len(), 3);
    EXPECT_TRUE(set_from_range.Contains(10));
    EXPECT_TRUE(set_from_range.Contains(20));
    EXPECT_TRUE(set_from_range.Contains(30));
}

TEST_F(HashSetAPI_Test, AddAndContains)
{
    HashSet<String> set;
    EXPECT_TRUE(set.Insert("apple"));
    EXPECT_EQ(set.Len(), 1);
    EXPECT_TRUE(set.Contains("apple"));
    EXPECT_FALSE(set.Insert("apple")); // Already exists
    EXPECT_EQ(set.Len(), 1);

    set.Insert("banana");
    EXPECT_EQ(set.Len(), 2);
    EXPECT_TRUE(set.Contains("banana"));
}

TEST_F(HashSetAPI_Test, Emplace)
{
    struct TestStruct
    {
        String name;
        int id;

        TestStruct(String n, int i)
            : name(std::move(n))
            , id(i)
        {
        }

        bool operator==(const TestStruct& other) const { return name == other.name && id == other.id; }
    };

    // Custom hash for TestStruct
    struct TestStructHasher
    {
        usize operator()(const TestStruct& ts) const
        {
            return std::hash<String>()(ts.name) ^ std::hash<int>()(ts.id);
        }
    };

    HashSet<TestStruct, TestStructHasher> set;
    EXPECT_TRUE(set.Emplace("item1", 1));
    EXPECT_EQ(set.Len(), 1);
    EXPECT_TRUE(set.Contains(TestStruct("item1", 1)));

    EXPECT_FALSE(set.Emplace("item1", 1)); // Already exists
    EXPECT_EQ(set.Len(), 1);

    EXPECT_TRUE(set.Emplace("item2", 2));
    EXPECT_EQ(set.Len(), 2);
}

TEST_F(HashSetAPI_Test, Remove)
{
    HashSet<int> set = { 1, 2, 3 };
    EXPECT_TRUE(set.Remove(2));
    EXPECT_EQ(set.Len(), 2);
    EXPECT_FALSE(set.Contains(2));
    EXPECT_FALSE(set.Remove(4)); // Not found
    EXPECT_EQ(set.Len(), 2);
}

TEST_F(HashSetAPI_Test, RemoveIf)
{
    HashSet<int> set = { 1, 2, 3, 4, 5, 6 };
    usize removed_count = set.RemoveIf([](int val) { return val % 2 == 0; }); // Remove even numbers
    EXPECT_EQ(removed_count, 3);
    EXPECT_EQ(set.Len(), 3);
    EXPECT_TRUE(set.Contains(1));
    EXPECT_TRUE(set.Contains(3));
    EXPECT_TRUE(set.Contains(5));
    EXPECT_FALSE(set.Contains(2));
}

TEST_F(HashSetAPI_Test, Clear)
{
    HashSet<int> set = { 1, 2, 3 };
    set.Clear();
    EXPECT_TRUE(set.IsEmpty());
    EXPECT_EQ(set.Len(), 0);
    EXPECT_GT(set.Capacity(), 0); // Capacity is not necessarily 0 after clear
}

TEST_F(HashSetAPI_Test, CapacityAndReserve)
{
    HashSet<int> set;
    set.Reserve(10);
    EXPECT_GE(set.Capacity(), 10);
    set.Insert(1);
    EXPECT_EQ(set.Len(), 1);
}

TEST_F(HashSetAPI_Test, ToArray)
{
    HashSet<int> set = { 3, 1, 2 };
    Array<int> arr = set.ToArray();
    EXPECT_EQ(arr.Len(), 3);
    // Order is not guaranteed, so EXPECT_TRUE for presence
    EXPECT_TRUE(arr.Contains(1));
    EXPECT_TRUE(arr.Contains(2));
    EXPECT_TRUE(arr.Contains(3));
}

TEST_F(HashSetAPI_Test, IteratorsAndRangeBasedForLoop)
{
    HashSet<int> set = { 10, 20, 30 };
    int sum = 0;
    for (int val : set)
    {
        sum += val;
    }
    EXPECT_EQ(sum, 60);

    const HashSet<int> const_set = { 40, 50 };
    int const_sum = 0;
    for (int val : const_set)
    {
        const_sum += val;
    }
    EXPECT_EQ(const_sum, 90);
}

TEST_F(HashSetAPI_Test, CopyAndMoveSemantics)
{
    HashSet<int> set1 = { 1, 2, 3 };
    HashSet<int> set2 = set1; // Copy constructor
    EXPECT_EQ(set2.Len(), 3);
    EXPECT_TRUE(set2.Contains(1));

    HashSet<int> set3;
    set3 = set1; // Copy assignment
    EXPECT_EQ(set3.Len(), 3);
    EXPECT_TRUE(set3.Contains(2));

    HashSet<int> set4 = std::move(set1); // Move constructor
    EXPECT_EQ(set4.Len(), 3);
    EXPECT_TRUE(set4.Contains(3));
    EXPECT_TRUE(set1.IsEmpty()); // Moved-from state

    HashSet<int> set5;
    set5 = std::move(set3); // Move assignment
    EXPECT_EQ(set5.Len(), 3);
    EXPECT_TRUE(set5.Contains(1));
    EXPECT_TRUE(set3.IsEmpty());
}

TEST_F(SetAPI_Test, DefaultConstruction)
{
    Set<int> set;
    EXPECT_TRUE(set.IsEmpty());
    EXPECT_EQ(set.Len(), 0);
}

TEST_F(SetAPI_Test, InitializerListConstruction)
{
    Set<int> set = { 3, 1, 2, 1, 3 };
    EXPECT_EQ(set.Len(), 3);
    EXPECT_TRUE(set.Contains(1));
    EXPECT_TRUE(set.Contains(2));
    EXPECT_TRUE(set.Contains(3));
    EXPECT_FALSE(set.Contains(4));
}

TEST_F(SetAPI_Test, ConstructionFromIteratorsAndRange)
{
    std::vector<int> vec = { 30, 10, 20, 10 };
    Set<int> set_from_iter(vec.begin(), vec.end());
    EXPECT_EQ(set_from_iter.Len(), 3);
    EXPECT_TRUE(set_from_iter.Contains(10));
    EXPECT_TRUE(set_from_iter.Contains(20));
    EXPECT_TRUE(set_from_iter.Contains(30));

    Set<int> set_from_range = Set<int>::FromRange(vec);
    EXPECT_EQ(set_from_range.Len(), 3);
    EXPECT_TRUE(set_from_range.Contains(10));
    EXPECT_TRUE(set_from_range.Contains(20));
    EXPECT_TRUE(set_from_range.Contains(30));
}

TEST_F(SetAPI_Test, AddAndContains)
{
    Set<String> set;
    EXPECT_TRUE(set.Insert("apple"));
    EXPECT_EQ(set.Len(), 1);
    EXPECT_TRUE(set.Contains("apple"));
    EXPECT_FALSE(set.Insert("apple")); // Already exists
    EXPECT_EQ(set.Len(), 1);

    set.Insert("banana");
    EXPECT_EQ(set.Len(), 2);
    EXPECT_TRUE(set.Contains("banana"));
}

TEST_F(SetAPI_Test, Emplace)
{
    struct TestStruct
    {
        String name;
        int id;

        TestStruct(String n, int i)
            : name(std::move(n))
            , id(i)
        {
        }

        bool operator<(const TestStruct& other) const
        {
            if (name != other.name) return name < other.name;
            return id < other.id;
        }

        bool operator==(const TestStruct& other) const { return name == other.name && id == other.id; }
    };

    Set<TestStruct> set;
    EXPECT_TRUE(set.Emplace("item1", 1));
    EXPECT_EQ(set.Len(), 1);
    EXPECT_TRUE(set.Contains(TestStruct("item1", 1)));

    EXPECT_FALSE(set.Emplace("item1", 1)); // Already exists
    EXPECT_EQ(set.Len(), 1);

    EXPECT_TRUE(set.Emplace("item2", 2));
    EXPECT_EQ(set.Len(), 2);
}

TEST_F(SetAPI_Test, Remove)
{
    Set<int> set = { 1, 2, 3 };
    EXPECT_TRUE(set.Remove(2));
    EXPECT_EQ(set.Len(), 2);
    EXPECT_FALSE(set.Contains(2));
    EXPECT_FALSE(set.Remove(4)); // Not found
    EXPECT_EQ(set.Len(), 2);
}

TEST_F(SetAPI_Test, RemoveIf)
{
    Set<int> set = { 1, 2, 3, 4, 5, 6 };
    usize removed_count = set.RemoveIf([](int val) { return val % 2 == 0; }); // Remove even numbers
    EXPECT_EQ(removed_count, 3);
    EXPECT_EQ(set.Len(), 3);
    EXPECT_TRUE(set.Contains(1));
    EXPECT_TRUE(set.Contains(3));
    EXPECT_TRUE(set.Contains(5));
    EXPECT_FALSE(set.Contains(2));
}

TEST_F(SetAPI_Test, Clear)
{
    Set<int> set = { 1, 2, 3 };
    set.Clear();
    EXPECT_TRUE(set.IsEmpty());
    EXPECT_EQ(set.Len(), 0);
}

TEST_F(SetAPI_Test, ToArray)
{
    Set<int> set = { 3, 1, 2 };
    Array<int> arr = set.ToArray();
    EXPECT_EQ(arr.Len(), 3);
    // Order is guaranteed for Set
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

TEST_F(SetAPI_Test, IteratorsAndRangeBasedForLoop)
{
    Set<int> set = { 30, 10, 20 };
    int sum = 0;
    String concat_str;
    for (int val : set) // Should iterate in order: 10, 20, 30
    {
        sum += val;
        concat_str.Append(String::Format("{}", val));
    }
    EXPECT_EQ(sum, 60);
    EXPECT_EQ(concat_str, "102030");

    const Set<int> const_set = { 50, 40 };
    int const_sum = 0;
    for (int val : const_set)
    {
        const_sum += val;
    }
    EXPECT_EQ(const_sum, 90);
}

TEST_F(SetAPI_Test, CopyAndMoveSemantics)
{
    Set<int> set1 = { 1, 2, 3 };
    Set<int> set2 = set1; // Copy constructor
    EXPECT_EQ(set2.Len(), 3);
    EXPECT_TRUE(set2.Contains(1));

    Set<int> set3;
    set3 = set1; // Copy assignment
    EXPECT_EQ(set3.Len(), 3);
    EXPECT_TRUE(set3.Contains(2));

    Set<int> set4 = std::move(set1); // Move constructor
    EXPECT_EQ(set4.Len(), 3);
    EXPECT_TRUE(set4.Contains(3));
    EXPECT_TRUE(set1.IsEmpty()); // Moved-from state

    Set<int> set5;
    set5 = std::move(set3); // Move assignment
    EXPECT_EQ(set5.Len(), 3);
    EXPECT_TRUE(set5.Contains(1));
    EXPECT_TRUE(set3.IsEmpty());
}

TEST_F(PriorityQueueAPI_Test, DefaultConstructionAndPushPop_Max_Heap)
{
    PriorityQueue<int> pq;
    EXPECT_TRUE(pq.IsEmpty());
    EXPECT_EQ(pq.Len(), 0);

    pq.Push(10); // {10}
    pq.Push(30); // {30, 10}
    pq.Push(20); // {30, 10, 20} (heap property)
    pq.Push(5);  // {30, 10, 20, 5} (heap property)

    EXPECT_FALSE(pq.IsEmpty());
    EXPECT_EQ(pq.Len(), 4);
    EXPECT_EQ(*pq.Peek(), 30);

    EXPECT_EQ(*pq.Pop(), 30); // {20, 10, 5}
    EXPECT_EQ(pq.Len(), 3);
    EXPECT_EQ(*pq.Peek(), 20);

    EXPECT_EQ(*pq.Pop(), 20); // {10, 5}
    EXPECT_EQ(*pq.Pop(), 10); // {5}
    EXPECT_EQ(*pq.Pop(), 5);  // {}
    EXPECT_TRUE(pq.IsEmpty());
    EXPECT_FALSE(pq.Pop().HasValue());
}

TEST_F(PriorityQueueAPI_Test, ConstructionFromIterators)
{
    std::vector<int> vec = { 10, 30, 20, 5 };
    PriorityQueue<int> pq(vec.begin(), vec.end());
    EXPECT_EQ(pq.Len(), 4);
    EXPECT_EQ(*pq.Peek(), 30); // Max element
}

TEST_F(PriorityQueueAPI_Test, Emplace)
{
    struct TestStruct
    {
        int priority;
        String name;

        TestStruct(int p, String n)
            : priority(p)
            , name(std::move(n))
        {
        }

        bool operator<(const TestStruct& other) const { return priority < other.priority; }
    };

    PriorityQueue<TestStruct> pq;
    pq.Emplace(10, "low");
    pq.Emplace(30, "high");
    pq.Emplace(20, "medium");

    EXPECT_EQ(pq.Len(), 3);
    EXPECT_EQ(pq.Peek()->priority, 30);
    EXPECT_EQ(pq.Peek()->name, "high");

    EXPECT_EQ(pq.Pop()->priority, 30);
    EXPECT_EQ(pq.Pop()->priority, 20);
    EXPECT_EQ(pq.Pop()->priority, 10);
}

TEST_F(PriorityQueueAPI_Test, PushRange)
{
    PriorityQueue<int> pq;
    std::vector<int> values = { 1, 5, 2, 8, 3 };
    pq.PushRange(values);
    EXPECT_EQ(pq.Len(), 5);
    EXPECT_EQ(*pq.Peek(), 8);
    EXPECT_EQ(*pq.Pop(), 8);
    EXPECT_EQ(*pq.Pop(), 5);
}

TEST_F(PriorityQueueAPI_Test, Clear)
{
    PriorityQueue<int> pq = { 1, 2, 3 };
    pq.Clear();
    EXPECT_TRUE(pq.IsEmpty());
    EXPECT_EQ(pq.Len(), 0);
    EXPECT_FALSE(pq.Peek().HasValue());
}

TEST_F(PriorityQueueAPI_Test, ToUnderlyingContainer)
{
    PriorityQueue<int> pq = { 10, 30, 20 };
    Array<int> arr = pq.ToUnderlyingContainer();
    EXPECT_EQ(arr.Len(), 3);
    // The underlying container is a heap, not necessarily sorted
    // So we EXPECT_TRUE if the elements are present
    EXPECT_TRUE(arr.Contains(10));
    EXPECT_TRUE(arr.Contains(20));
    EXPECT_TRUE(arr.Contains(30));
}

TEST_F(PriorityQueueAPI_Test, Min_Heap_CustomCompare)
{
    PriorityQueue<int, Array<int>, std::greater<>> min_pq; // Use std::greater for min-heap
    min_pq.Push(10);
    min_pq.Push(30);
    min_pq.Push(20);
    min_pq.Push(5);

    EXPECT_EQ(*min_pq.Peek(), 5);
    EXPECT_EQ(*min_pq.Pop(), 5);
    EXPECT_EQ(*min_pq.Pop(), 10);
    EXPECT_EQ(*min_pq.Pop(), 20);
    EXPECT_EQ(*min_pq.Pop(), 30);
    EXPECT_TRUE(min_pq.IsEmpty());
}

TEST_F(PriorityQueueAPI_Test, Swap)
{
    PriorityQueue<int> pq1 = { 1, 5, 2 };
    PriorityQueue<int> pq2 = { 10, 30, 20 };

    pq1.Swap(pq2);

    EXPECT_EQ(pq1.Len(), 3);
    EXPECT_EQ(*pq1.Peek(), 30);

    EXPECT_EQ(pq2.Len(), 3);
    EXPECT_EQ(*pq2.Peek(), 5);
}
