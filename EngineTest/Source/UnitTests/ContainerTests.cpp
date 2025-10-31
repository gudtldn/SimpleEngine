#include "doctest/doctest.h"

// ReSharper disable once CppUnusedIncludeDirective
#include <iostream>
#include <vector>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/LinkedList.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Container/PriorityQueue.h"
#include "SimpleEngine/Core/Container/Set.h"
#include "SimpleEngine/Core/Container/String.h"


// Using the namespace where containers are defined
using namespace se;


TEST_SUITE("SimpleEngine Basic Container API")
{
TEST_CASE("FixedArray API")
{
    SUBCASE("Default Construction and Capacity")
    {
        FixedArray<int, 5> arr;
        CHECK(arr.Len() == 5);
        CHECK_FALSE(arr.IsEmpty());
    }

    SUBCASE("Element Access with At()")
    {
        FixedArray<int, 3> arr;
        *arr.At(0) = 10;
        *arr.At(1) = 20;
        *arr.At(2) = 30;

        CHECK(arr.At(0).HasValue());
        CHECK(*arr.At(0) == 10);

        CHECK(arr.At(1).HasValue());
        CHECK(*arr.At(1) == 20);

        CHECK(arr.At(2).HasValue());
        CHECK(*arr.At(2) == 30);

        CHECK_FALSE(arr.At(3).HasValue());
        CHECK_FALSE(arr.At(static_cast<usize>(-1)).HasValue());
    }

    SUBCASE("Copy Construction and Assignment")
    {
        FixedArray<int, 3> arr1;
        *arr1.At(0) = 1;
        *arr1.At(1) = 2;
        *arr1.At(2) = 3;

        FixedArray<int, 3> arr2 = arr1; // 복사 생성
        CHECK(*arr2.At(0) == 1);
        CHECK(*arr2.At(1) == 2);
        CHECK(*arr2.At(2) == 3);

        FixedArray<int, 3> arr3;
        arr3 = arr1; // 복사 할당
        CHECK(*arr3.At(0) == 1);
        CHECK(*arr3.At(1) == 2);
        CHECK(*arr3.At(2) == 3);
    }

    SUBCASE("Range-based for loop")
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
        CHECK(sum == 10);
    }

    SUBCASE("Constexpr Operations")
    {
        // FixedArray의 생성 및 기본 메서드가 constexpr로 동작하는지 확인
        constexpr FixedArray<int, 3> const_arr{}; // {}를 사용하여 집계 초기화
        static_assert(const_arr.Len() == 3, "Len() should be constexpr");
        static_assert(!const_arr.IsEmpty(), "IsEmpty() should be constexpr");

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
}

TEST_CASE("Array API")
{
    SUBCASE("Default Construction")
    {
        Array<int> arr;
        CHECK(arr.Len() == 0);
        CHECK(arr.IsEmpty());
        CHECK(arr.Capacity() == 0);
    }

    SUBCASE("Uninitialized")
    {
        // This is only safe for trivially default constructible types
        Array<int> arr = Array<int>::Uninitialized(5);
        CHECK(arr.Len() == 5);
        CHECK(arr.Capacity() >= 5);
        // The values are uninitialized, so we just write to them
        arr[0] = 1;
        arr[4] = 5;
        CHECK(arr[0] == 1);
        CHECK(arr[4] == 5);
    }

    SUBCASE("Construction with size")
    {
        Array<int> arr(5);
        CHECK(arr.Len() == 5);
        CHECK_FALSE(arr.IsEmpty());
        CHECK(arr.Capacity() >= 5);
        // Elements are default-initialized
        CHECK(arr[0] == 0);
    }

    SUBCASE("Construction with size and value")
    {
        Array<int> arr(3, 10);
        CHECK(arr.Len() == 3);
        CHECK(arr[0] == 10);
        CHECK(arr[1] == 10);
        CHECK(arr[2] == 10);
    }

    SUBCASE("Initializer List Construction")
    {
        Array<int> arr = { 1, 2, 3, 4, 5 };
        CHECK(arr.Len() == 5);
        CHECK(arr[0] == 1);
        CHECK(arr[4] == 5);
    }

    SUBCASE("Construction from range")
    {
        std::vector<int> vec = { 1, 2, 3, 4, 5 };
        Array<int> arr_from_vec(vec.begin(), vec.end());
        CHECK(arr_from_vec.Len() == 5);
        CHECK(arr_from_vec[0] == 1);
        CHECK(arr_from_vec[4] == 5);

        Array<int> arr_from_range = Array<int>::FromRange(vec);
        CHECK(arr_from_range.Len() == 5);
        CHECK(arr_from_range[0] == 1);
        CHECK(arr_from_range[4] == 5);
    }

    SUBCASE("Push and Pop")
    {
        Array<int> arr;
        usize index1 = arr.Push(10);
        usize index2 = arr.Push(20);
        CHECK(arr.Len() == 2);
        CHECK(arr[1] == 20);
        CHECK(index1 == 0);
        CHECK(index2 == 1);

        auto popped = arr.Pop();
        CHECK(popped.HasValue());
        CHECK(*popped == 20);
        CHECK(arr.Len() == 1);

        popped = arr.Pop();
        CHECK(popped.HasValue());
        CHECK(*popped == 10);
        CHECK(arr.IsEmpty());

        popped = arr.Pop();
        CHECK_FALSE(popped.HasValue());
    }

    SUBCASE("Element Access")
    {
        Array<int> arr = { 10, 20, 30 };
        CHECK(*arr.At(1) == 20);
        CHECK_FALSE(arr.At(3).HasValue());

        CHECK(*arr.Front() == 10);
        CHECK(*arr.Back() == 30);

        arr[0] = 15;
        CHECK(arr[0] == 15);

        const Array<int> const_arr = { 1, 2 };
        CHECK(*const_arr.At(0) == 1);
        CHECK(*const_arr.Front() == 1);
        CHECK(*const_arr.Back() == 2);
        CHECK(const_arr[1] == 2);
    }

    SUBCASE("Data")
    {
        Array<int> arr = { 1, 2, 3 };
        int* data_ptr = arr.Data();
        CHECK(data_ptr[0] == 1);
        CHECK(data_ptr[1] == 2);
        CHECK(data_ptr[2] == 3);

        const Array<int> const_arr = { 4, 5, 6 };
        const int* const_data_ptr = const_arr.Data();
        CHECK(const_data_ptr[0] == 4);
        CHECK(const_data_ptr[1] == 5);
        CHECK(const_data_ptr[2] == 6);
    }

    SUBCASE("Empty Array Access")
    {
        Array<int> arr;
        CHECK_FALSE(arr.Front().HasValue());
        CHECK_FALSE(arr.Back().HasValue());
        CHECK_FALSE(arr.At(0).HasValue());
    }

    SUBCASE("Capacity and Reserving")
    {
        Array<int> arr;
        arr.Reserve(10);
        CHECK(arr.Capacity() >= 10);
        CHECK(arr.Len() == 0);

        arr.Push(1);
        arr.ShrinkToFit();
        CHECK(arr.Capacity() == 1);
        CHECK(arr.Len() == 1);
    }

    SUBCASE("Resize")
    {
        Array<int> arr = { 1, 2, 3 };
        arr.Resize(5);
        CHECK(arr.Len() == 5);
        CHECK(arr[2] == 3);
        // New elements are default-initialized
        CHECK(arr[3] == 0);
        CHECK(arr[4] == 0);

        arr.Resize(2);
        CHECK(arr.Len() == 2);
        CHECK(arr[1] == 2);

        // Test resize with value
        arr.Resize(4, 99);
        CHECK(arr.Len() == 4);
        CHECK(arr[1] == 2);
        CHECK(arr[2] == 99);
        CHECK(arr[3] == 99);
    }

    SUBCASE("Truncate")
    {
        Array<int> arr = { 1, 2, 3, 4, 5 };
        arr.Truncate(3);
        CHECK(arr.Len() == 3);
        CHECK(arr[0] == 1);
        CHECK(arr[1] == 2);
        CHECK(arr[2] == 3);

        // Truncate to a larger or equal size should do nothing
        arr.Truncate(5);
        CHECK(arr.Len() == 3);
    }

    SUBCASE("Clear")
    {
        Array<int> arr = { 1, 2, 3 };
        arr.Clear();
        CHECK(arr.Len() == 0);
        CHECK(arr.IsEmpty());
        CHECK(arr.Capacity() >= 3); // Capacity is not changed
    }

    SUBCASE("Insert and Remove")
    {
        Array<int> arr = { 10, 20, 30 };
        arr.Insert(1, 15); // {10, 15, 20, 30}
        CHECK(arr.Len() == 4);
        CHECK(arr[1] == 15);
        CHECK(arr[2] == 20);

        arr.RemoveAt(2); // {10, 15, 30}
        CHECK(arr.Len() == 3);
        CHECK(arr[2] == 30);
    }

    SUBCASE("Insert range")
    {
        Array<int> arr = { 10, 40 };
        std::vector<int> to_insert = { 20, 30 };

        // Test with iterators
        arr.Insert(1, to_insert.begin(), to_insert.end());
        CHECK(arr.Len() == 4);
        CHECK(arr[0] == 10);
        CHECK(arr[1] == 20);
        CHECK(arr[2] == 30);
        CHECK(arr[3] == 40);

        // Test with range
        Array<int> arr2 = { 10, 40 };
        arr2.InsertRange(1, to_insert);
        CHECK(arr2.Len() == 4);
        CHECK(arr2[0] == 10);
        CHECK(arr2[1] == 20);
        CHECK(arr2[2] == 30);
        CHECK(arr2[3] == 40);
    }

    SUBCASE("Remove")
    {
        Array<int> arr = { 1, 2, 1, 1, 2, 3, 1 };
        usize removed_count = arr.Remove(1);
        CHECK(removed_count == 4);
        CHECK(arr.Len() == 3);
        CHECK(arr[0] == 2);
        CHECK(arr[1] == 2);
        CHECK(arr[2] == 3);
    }


    SUBCASE("RemoveRange")
    {
        Array<int> arr = { 10, 20, 30, 40, 50, 60 };
        arr.RemoveRange(2, 3); // Remove 30, 40, 50
        CHECK(arr.Len() == 3);
        CHECK(arr[0] == 10);
        CHECK(arr[1] == 20);
        CHECK(arr[2] == 60);
    }

    SUBCASE("RemoveIf")
    {
        Array<int> arr = { 1, 2, 3, 4, 5, 6 };
        usize removed_count = arr.RemoveIf([](int val) { return val % 2 == 0; }); // Remove even numbers
        CHECK(removed_count == 3);
        CHECK(arr.Len() == 3);
        CHECK(arr[0] == 1);
        CHECK(arr[1] == 3);
        CHECK(arr[2] == 5);
    }

    SUBCASE("RemoveAtSwap")
    {
        Array<int> arr = { 10, 20, 30, 40 };
        bool success = arr.RemoveAtSwap(1); // swap 20 with 40, then pop
        CHECK(success);
        CHECK(arr.Len() == 3);
        CHECK(arr[0] == 10);
        CHECK(arr[1] == 40); // 20 was swapped with 40
        CHECK(arr[2] == 30);

        success = arr.RemoveAtSwap(5); // out of bounds
        CHECK_FALSE(success);

        // Test removing the last element
        Array<int> arr2 = { 1, 2, 3 };
        success = arr2.RemoveAtSwap(2);
        CHECK(success);
        CHECK(arr2.Len() == 2);
        CHECK(arr2[0] == 1);
        CHECK(arr2[1] == 2);
    }

    SUBCASE("Contains and Find")
    {
        Array<int> arr = { 10, 20, 30, 20 };
        CHECK(arr.Contains(20));
        CHECK_FALSE(arr.Contains(50));

        auto index = arr.Find(20);
        CHECK(index.HasValue());
        CHECK(*index == 1); // Finds first occurrence

        index = arr.Find(50);
        CHECK_FALSE(index.HasValue());
    }

    SUBCASE("Push")
    {
        Array<int> arr1 = { 1, 2 };
        Array<int> arr2 = { 3, 4 };
        arr1.PushRange(arr2);
        CHECK(arr1.Len() == 4);
        CHECK(arr1[2] == 3);
        CHECK(arr1[3] == 4);

        std::vector<int> vec = { 5, 6 };
        arr1.PushRange(vec);
        CHECK(arr1.Len() == 6);
        CHECK(arr1[4] == 5);
        CHECK(arr1[5] == 6);
    }

    SUBCASE("Emplace")
    {
        struct TestStruct
        {
            int x;
            double y;

            TestStruct(int x, double y)
                : x(x)
                , y(y)
            {
            }
        };
        Array<TestStruct> arr;
        usize index = arr.Emplace(1, 3.14);
        CHECK(arr.Len() == 1);
        CHECK(arr[0].x == 1);
        CHECK(arr[0].y == 3.14);
        CHECK(index == 0);
    }

    SUBCASE("Range-based for loop")
    {
        Array<int> arr = { 1, 2, 3, 4 };
        int sum = 0;
        for (const int val : arr)
        {
            sum += val;
        }
        CHECK(sum == 10);
    }

    SUBCASE("Copy and Move Semantics")
    {
        Array<int> arr1 = { 1, 2, 3 };
        Array<int> arr2 = arr1; // Copy constructor
        CHECK(arr1.Len() == 3);
        CHECK(arr2.Len() == 3);
        CHECK(arr2[1] == 2);

        Array<int> arr3;
        arr3 = arr1; // Copy assignment
        CHECK(arr3.Len() == 3);
        CHECK(arr3[1] == 2);

        Array<int> arr4 = std::move(arr1); // Move constructor
        CHECK(arr4.Len() == 3);
        CHECK(arr4[1] == 2);
        // arr1 is in a valid but unspecified state, but should be empty
        CHECK(arr1.IsEmpty());

        Array<int> arr5;
        arr5 = std::move(arr3); // Move assignment
        CHECK(arr5.Len() == 3);
        CHECK(arr5[1] == 2);
        CHECK(arr3.IsEmpty());
    }
}

TEST_CASE("String API")
{
    SUBCASE("Construction")
    {
        String s1;
        CHECK(s1.IsEmpty());
        CHECK(s1.ByteLen() == 0);
        CHECK(s1.CodePointLen() == 0);

        String s2("hello");
        CHECK(s2.ByteLen() == 5);
        CHECK(s2.CodePointLen() == 5);
        CHECK(s2 == "hello");

        // UTF-8 string: "안녕하세요" (5 characters, 15 bytes)
        String s3("안녕하세요");
        CHECK(s3.ByteLen() == 15);
        CHECK(s3.CodePointLen() == 5);
        CHECK(s3 == "안녕하세요");

        String s4(U'😊', 3); // 3 smiling faces
        CHECK(s4 == "😊😊😊");
        CHECK(s4.ByteLen() == 12); // 4 bytes per emoji
        CHECK(s4.CodePointLen() == 3);

        String s5(std::string_view("world"));
        CHECK(s5 == "world");

        String s6 = s5; // Copy construction
        CHECK(s6 == "world");

        String s7 = std::move(s6); // Move construction
        CHECK(s7 == "world");
        CHECK(s6.IsEmpty()); // Moved-from state
    }

    SUBCASE("Assignment")
    {
        String s;
        s = "test";
        CHECK(s == "test");
        s = std::string_view("another");
        CHECK(s == "another");
        s = U'X';
        CHECK(s == "X");
        CHECK(s.ByteLen() == 1);
        CHECK(s.CodePointLen() == 1);
    }

    SUBCASE("Push and Concatenation")
    {
        String s1("Hello");
        s1.Append(" World");
        CHECK(s1 == "Hello World");

        String s2 = s1 + "!";
        CHECK(s2 == "Hello World!");

        s2 += U'😊';
        CHECK(s2 == "Hello World!😊");

        String s3 = "Prefix: " + s2;
        CHECK(s3.StartsWith("Prefix: "));
    }

    SUBCASE("Find and Contains")
    {
        String s("Hello World, Hello Universe");
        CHECK(s.Contains("World"));
        CHECK_FALSE(s.Contains("Galaxy"));

        CHECK(s.StartsWith("Hello"));
        CHECK(s.EndsWith("Universe"));

        auto find_res = s.Find("Hello");
        CHECK(find_res.HasValue());
        CHECK(*find_res == 0);

        find_res = s.Find("Hello", 1);
        CHECK(find_res.HasValue());
        CHECK(*find_res == 13);

        auto rfind_res = s.FindLast("Hello");
        CHECK(rfind_res.HasValue());
        CHECK(*rfind_res == 13);
    }

    SUBCASE("Substrings")
    {
        String s("0123456789");
        String sub1 = s.Substring(2, 5);
        CHECK(sub1 == "23456");

        String sub2 = s.Substring(5);
        CHECK(sub2 == "56789");

        std::string_view view1 = s.SubstringView(2, 5);
        CHECK(view1 == "23456");
    }

    SUBCASE("Insert and Remove")
    {
        String s("Hello Universe");
        s.Insert(6, "Beautiful ");
        CHECK(s == "Hello Beautiful Universe");

        // Remove "Beautiful " (10 bytes)
        s.RemoveRange(6, 10);
        CHECK(s == "Hello Universe");
    }

    SUBCASE("Push and Pop")
    {
        String s("abc");
        s.Push(U'😊');
        CHECK(s == "abc😊");

        Optional<char32> popped = s.Pop();
        CHECK(popped.HasValue());
        CHECK(*popped == U'😊');
        CHECK(s == "abc");

        s.Pop();
        s.Pop();
        s.Pop();
        CHECK(s.IsEmpty());

        popped = s.Pop();
        CHECK_FALSE(popped.HasValue());
    }

    SUBCASE("Case Conversion")
    {
        String s("Hello World");
        String upper = s.ToUpper();
        String lower = s.ToLower();
        CHECK(upper == "HELLO WORLD");
        CHECK(lower == "hello world");

        // Turkish 'i' test
        String turkish_i("Iıİi");
        String turkish_upper = turkish_i.ToUpper("tr_TR");
        String turkish_lower = turkish_i.ToLower("tr_TR");
        CHECK(turkish_upper == "IIİİ");
        CHECK(turkish_lower == "ııii");
    }

    SUBCASE("CodePoints and Bytes View")
    {
        String s("a😊b"); // a(1) + emoji(4) + b(1) = 6 bytes
        CHECK(s.ByteLen() == 6);
        CHECK(s.CodePointLen() == 3);

        int cp_count = 0;
        for ([[maybe_unused]] char32 cp : s.CodePoints())
        {
            cp_count++;
        }
        CHECK(cp_count == 3);

        int byte_count = 0;
        for ([[maybe_unused]] char byte : s.Bytes())
        {
            byte_count++;
        }
        CHECK(byte_count == 6);
    }

    SUBCASE("Comparison")
    {
        String s1("abc");
        String s2("abd");
        String s3("abc");

        CHECK(s1 == s3);
        CHECK(s1 != s2);
        CHECK(s1 < s2);
        CHECK(s2 > s1);
        CHECK(s1 <= s3);
    }

    SUBCASE("Static Format")
    {
        auto s = String::Format("The number is {} and the string is '{}'.", 42, "test");
        CHECK(s == "The number is 42 and the string is 'test'.");
    }
}

TEST_CASE("Deque API")
{
    SUBCASE("Construction")
    {
        // Default
        Deque<int> d1;
        CHECK(d1.IsEmpty());
        CHECK(d1.Len() == 0);

        // With size
        Deque<int> d2(5);
        CHECK(d2.Len() == 5);
        CHECK(d2[0] == 0);

        // With size and value
        Deque<int> d3(3, 100);
        CHECK(d3.Len() == 3);
        CHECK(d3[0] == 100);
        CHECK(d3[1] == 100);
        CHECK(d3[2] == 100);

        // Initializer list
        Deque<int> d4 = { 1, 2, 3 };
        CHECK(d4.Len() == 3);
        CHECK(d4[1] == 2);

        // From iterators
        std::vector<int> vec = { 4, 5, 6 };
        Deque<int> d5(vec.begin(), vec.end());
        CHECK(d5.Len() == 3);
        CHECK(d5[1] == 5);

        // From range
        Deque<int> d6 = Deque<int>::FromRange(vec);
        CHECK(d6.Len() == 3);
        CHECK(d6[1] == 5);
    }

    SUBCASE("Push and Pop")
    {
        Deque<int> d;
        d.PushBack(10);
        d.PushFront(20); // {20, 10}
        d.PushBack(30);  // {20, 10, 30}
        CHECK(d.Len() == 3);
        CHECK(d[0] == 20);
        CHECK(d[1] == 10);
        CHECK(d[2] == 30);

        auto pop_back = d.PopBack();
        CHECK(pop_back.HasValue());
        CHECK(*pop_back == 30);
        CHECK(d.Len() == 2);

        auto pop_front = d.PopFront();
        CHECK(pop_front.HasValue());
        CHECK(*pop_front == 20);
        CHECK(d.Len() == 1);

        d.PopBack();
        CHECK(d.IsEmpty());

        CHECK_FALSE(d.PopFront().HasValue());
        CHECK_FALSE(d.PopBack().HasValue());
    }

    SUBCASE("Emplace")
    {
        struct TestStruct
        {
            int x;
            double y;
            TestStruct(int x, double y) : x(x), y(y) {}
        };

        Deque<TestStruct> d;
        d.EmplaceBack(1, 1.1);
        d.EmplaceFront(2, 2.2);
        CHECK(d.Len() == 2);
        CHECK(d[0].x == 2);
        CHECK(d[1].x == 1);
    }

    SUBCASE("Element Access")
    {
        Deque<int> d = { 10, 20, 30 };
        CHECK(d[1] == 20);

        *d.At(0) = 15;
        CHECK(*d.At(0) == 15);
        CHECK_FALSE(d.At(3).HasValue());

        CHECK(*d.Front() == 15);
        CHECK(*d.Back() == 30);

        const Deque<int> cd = d;
        CHECK(cd[1] == 20);
        CHECK(*cd.At(0) == 15);
        CHECK(*cd.Front() == 15);
        CHECK(*cd.Back() == 30);
    }

    SUBCASE("Resize and Clear")
    {
        Deque<int> d = { 1, 2, 3, 4, 5 };
        d.Resize(3);
        CHECK(d.Len() == 3);
        CHECK(d[2] == 3);

        d.Resize(5, 100);
        CHECK(d.Len() == 5);
        CHECK(d[3] == 100);
        CHECK(d[4] == 100);

        d.Clear();
        CHECK(d.IsEmpty());
        CHECK(d.Len() == 0);
    }

    SUBCASE("ShrinkToFit")
    {
        Deque<int> d;
        d.PushBack(1);
        d.PushBack(2);
        d.PushBack(3);
        // NOTE: std::deque doesn't have capacity(), so we can't directly test if it shrank.
        // We just call it to ensure it compiles and doesn't crash.
        d.ShrinkToFit();
        CHECK(d.Len() == 3);
    }

    SUBCASE("Insert")
    {
        Deque<int> d = { 10, 50 };
        d.Insert(1, 20); // {10, 20, 50}
        CHECK(d.Len() == 3);
        CHECK(d[1] == 20);

        std::vector<int> vec = { 30, 40 };
        d.InsertRange(2, vec); // {10, 20, 30, 40, 50}
        CHECK(d.Len() == 5);
        CHECK(d[2] == 30);
        CHECK(d[3] == 40);
    }

    SUBCASE("Remove")
    {
        Deque<int> d = { 10, 20, 30, 40, 50 };
        d.RemoveAt(1); // remove 20 -> {10, 30, 40, 50}
        CHECK(d.Len() == 4);
        CHECK(d[1] == 30);

        Deque<int> d2 = { 1, 2, 1, 3, 1 };
        auto removed_count = d2.Remove(1);
        CHECK(removed_count == 3);
        CHECK(d2.Len() == 2);
        CHECK(d2[0] == 2);
        CHECK(d2[1] == 3);

        Deque<int> d3 = { 1, 2, 3, 4, 5, 6 };
        removed_count = d3.RemoveIf([](int val) { return val % 2 != 0; }); // remove odd
        CHECK(removed_count == 3);
        CHECK(d3.Len() == 3);
        CHECK(d3[0] == 2);
        CHECK(d3[1] == 4);
        CHECK(d3[2] == 6);
    }

    SUBCASE("Contains")
    {
        Deque<int> d = { 10, 20, 30 };
        CHECK(d.Contains(20));
        CHECK_FALSE(d.Contains(99));
    }

    SUBCASE("Swap")
    {
        Deque<int> d1 = { 1, 2, 3 };
        Deque<int> d2 = { 4, 5 };
        d1.Swap(d2);
        CHECK(d1.Len() == 2);
        CHECK(d1[0] == 4);
        CHECK(d2.Len() == 3);
        CHECK(d2[0] == 1);
    }

    SUBCASE("Range-based for loop")
    {
        Deque<int> d = { 1, 2, 3, 4 };
        int sum = 0;
        for (const int val : d)
        {
            sum += val;
        }
        CHECK(sum == 10);
    }

    SUBCASE("Copy and Move Semantics")
    {
        Deque<int> d1 = { 1, 2, 3 };
        Deque<int> d2 = d1; // Copy constructor
        CHECK(d1.Len() == 3);
        CHECK(d2.Len() == 3);
        CHECK(d2[1] == 2);

        Deque<int> d3;
        d3 = d1; // Copy assignment
        CHECK(d3.Len() == 3);
        CHECK(d3[1] == 2);

        Deque<int> d4 = std::move(d1); // Move constructor
        CHECK(d4.Len() == 3);
        CHECK(d4[1] == 2);
        CHECK(d1.IsEmpty()); // Moved-from state for std::deque is empty

        Deque<int> d5;
        d5 = std::move(d3); // Move assignment
        CHECK(d5.Len() == 3);
        CHECK(d5[1] == 2);
        CHECK(d3.IsEmpty());
    }
}

TEST_CASE("HashMap API")
{
    SUBCASE("Construction")
    {
        HashMap<String, int> map1;
        CHECK(map1.IsEmpty());
        CHECK(map1.Len() == 0);

        HashMap<String, int> map2 = { { "one", 1 }, { "two", 2 } };
        CHECK(map2.Len() == 2);
        CHECK(*map2.Find("one") == 1);

        std::vector<std::pair<const String, int>> vec = { { "three", 3 }, { "four", 4 } };
        HashMap<String, int> map3(vec.begin(), vec.end());
        CHECK(map3.Len() == 2);
        CHECK(*map3.Find("three") == 3);

        auto map4 = HashMap<String, int>::FromRange(vec);
        CHECK(map4.Len() == 2);
        CHECK(*map4.Find("four") == 4);
    }

    SUBCASE("Access and Modification")
    {
        HashMap<String, int> map;
        map["one"] = 1;
        CHECK(map.Len() == 1);
        CHECK(map["one"] == 1);

        map["one"] = 11;
        CHECK(map["one"] == 11);

        CHECK(map.Contains("one"));
        CHECK_FALSE(map.Contains("two"));

        auto find_res = map.Find("one");
        CHECK(find_res.HasValue());
        CHECK(*find_res == 11);

        auto find_res_const = static_cast<const decltype(map)&>(map).Find("one");
        CHECK(find_res_const.HasValue());
        CHECK(*find_res_const == 11);

        CHECK_FALSE(map.Find("two").HasValue());
    }

    SUBCASE("Remove and Clear")
    {
        HashMap<String, int> map = { { "one", 1 }, { "two", 2 }, { "three", 3 } };
        CHECK(map.Remove("two"));
        CHECK(map.Len() == 2);
        CHECK_FALSE(map.Contains("two"));
        CHECK_FALSE(map.Remove("four"));

        map.Clear();
        CHECK(map.IsEmpty());
        CHECK(map.Len() == 0);
        CHECK(map.Capacity() > 0); // Clear does not affect capacity
    }

    SUBCASE("Capacity and Reserve")
    {
        HashMap<String, int> map;
        // CHECK(map.Capacity() == 0);
        map.Reserve(10);
        CHECK(map.Capacity() >= 10);
        map["one"] = 1;
        CHECK(map.Len() == 1);
    }

    SUBCASE("Entry API")
    {
        HashMap<String, int> map;
        map["existing"] = 10;

        // Insert new entry
        auto entry1 = map.Entry("new");
        CHECK_FALSE(entry1.IsOccupied());
        int& val1 = entry1.OrInsert(20);
        CHECK(val1 == 20);
        CHECK(*map.Find("new") == 20);

        // Access existing entry
        auto entry2 = map.Entry("existing");
        CHECK(entry2.IsOccupied());
        int& val2 = entry2.OrInsert(0); // Should not insert
        CHECK(val2 == 10);
        *entry2.GetValue() = 11;
        CHECK(*map.Find("existing") == 11);

        // Modify with function
        auto entry3 = map.Entry("new");
        entry3.AndModify([](int& v) { v *= 2; });
        CHECK(*map.Find("new") == 40);
    }

    SUBCASE("GetKeys and GetValues")
    {
        HashMap<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
        Array<String> keys = map.GetKeys();
        Array<int> values = map.GetValues();

        CHECK(keys.Len() == 3);
        CHECK(values.Len() == 3);

        // The order is not guaranteed in HashMap, so we check for presence
        CHECK(keys.Contains("a"));
        CHECK(keys.Contains("b"));
        CHECK(keys.Contains("c"));

        CHECK(values.Contains(1));
        CHECK(values.Contains(2));
        CHECK(values.Contains(3));
    }

    SUBCASE("RemoveIf")
    {
        HashMap<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 } };
        usize removed_count = map.RemoveIf([]([[maybe_unused]] const String& key, const int& val)
        {
            return val % 2 == 0; // Remove even values
        });
        CHECK(removed_count == 2);
        CHECK(map.Len() == 2);
        CHECK(map.Contains("a"));
        CHECK_FALSE(map.Contains("b"));
        CHECK(map.Contains("c"));
        CHECK_FALSE(map.Contains("d"));
    }

    SUBCASE("Range-based for loop")
    {
        HashMap<String, int> map = { { "one", 1 }, { "two", 2 } };
        int sum = 0;
        for (auto& value : map | std::views::values)
        {
            sum += value;
        }
        CHECK(sum == 3);
    }

    SUBCASE("Emplace")
    {
        HashMap<String, String> map;

        // Emplace a new element
        String& val1 = map.Emplace("key1", "value1");
        CHECK(map.Len() == 1);
        CHECK(val1 == "value1");
        CHECK(*map.Find("key1") == "value1");

        // Emplace on existing key
        String& val2 = map.Emplace("key1", "value2_should_not_be_inserted");
        CHECK(map.Len() == 1);
        CHECK(val2 == "value1"); // Should return ref to existing value
        CHECK(*map.Find("key1") == "value1");

        // Modify through returned reference
        val2 = "modified_value";
        CHECK(*map.Find("key1") == "modified_value");
    }
}

TEST_CASE("Map API")
{
    SUBCASE("Construction")
    {
        Map<String, int> map1;
        CHECK(map1.IsEmpty());
        CHECK(map1.Len() == 0);

        Map<String, int> map2 = { { "one", 1 }, { "two", 2 } };
        CHECK(map2.Len() == 2);
        CHECK(*map2.Find("one") == 1);

        std::vector<std::pair<const String, int>> vec = { { "three", 3 }, { "four", 4 } };
        Map<String, int> map3(vec.begin(), vec.end());
        CHECK(map3.Len() == 2);
        CHECK(*map3.Find("three") == 3);

        auto map4 = Map<String, int>::FromRange(vec);
        CHECK(map4.Len() == 2);
        CHECK(*map4.Find("four") == 4);
    }

    SUBCASE("Access and Modification")
    {
        Map<String, int> map;
        map["one"] = 1;
        CHECK(map.Len() == 1);
        CHECK(map["one"] == 1);

        map["one"] = 11;
        CHECK(map["one"] == 11);

        CHECK(map.Contains("one"));
        CHECK_FALSE(map.Contains("two"));

        auto find_res = map.Find("one");
        CHECK(find_res.HasValue());
        CHECK(*find_res == 11);

        auto find_res_const = static_cast<const decltype(map)&>(map).Find("one");
        CHECK(find_res_const.HasValue());
        CHECK(*find_res_const == 11);

        CHECK_FALSE(map.Find("two").HasValue());
    }

    SUBCASE("Remove and Clear")
    {
        Map<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 } };
        CHECK(map.Remove("b"));
        CHECK(map.Len() == 2);
        CHECK_FALSE(map.Contains("b"));
        CHECK_FALSE(map.Remove("d"));

        map.Clear();
        CHECK(map.IsEmpty());
        CHECK(map.Len() == 0);
    }

    SUBCASE("Entry API")
    {
        Map<String, int> map;
        map["existing"] = 10;

        // Insert new entry
        auto entry1 = map.Entry("new");
        CHECK_FALSE(entry1.IsOccupied());
        int& val1 = entry1.OrInsert(20);
        CHECK(val1 == 20);
        CHECK(*map.Find("new") == 20);

        // Access existing entry
        auto entry2 = map.Entry("existing");
        CHECK(entry2.IsOccupied());
        int& val2 = entry2.OrInsert(0); // Should not insert
        CHECK(val2 == 10);
        *entry2.GetValue() = 11;
        CHECK(*map.Find("existing") == 11);

        // Modify with function
        auto entry3 = map.Entry("new");
        entry3.AndModify([](int& v) { v *= 2; });
        CHECK(*map.Find("new") == 40);
    }

    SUBCASE("GetKeys and GetValues")
    {
        Map<String, int> map = { { "c", 3 }, { "a", 1 }, { "b", 2 } };
        Array<String> keys = map.GetKeys();
        Array<int> values = map.GetValues();

        CHECK(keys.Len() == 3);
        CHECK(values.Len() == 3);

        // Map guarantees order
        CHECK(keys[0] == "a");
        CHECK(keys[1] == "b");
        CHECK(keys[2] == "c");

        CHECK(values[0] == 1);
        CHECK(values[1] == 2);
        CHECK(values[2] == 3);
    }

    SUBCASE("RemoveIf")
    {
        Map<String, int> map = { { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 } };
        usize removed_count = map.RemoveIf([]([[maybe_unused]] const String& key, const int& val)
        {
            return val % 2 == 0; // Remove even values
        });
        CHECK(removed_count == 2);
        CHECK(map.Len() == 2);
        CHECK(map.Contains("a"));
        CHECK_FALSE(map.Contains("b"));
        CHECK(map.Contains("c"));
        CHECK_FALSE(map.Contains("d"));
    }

    SUBCASE("Range-based for loop")
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
        CHECK(sum == 3);
        CHECK(key_concat == "onetwo");
    }

    SUBCASE("Emplace")
    {
        Map<String, String> map;

        // Emplace a new element
        String& val1 = map.Emplace("key1", "value1");
        CHECK(map.Len() == 1);
        CHECK(val1 == "value1");
        CHECK(*map.Find("key1") == "value1");

        // Emplace on existing key
        String& val2 = map.Emplace("key1", "value2_should_not_be_inserted");
        CHECK(map.Len() == 1);
        CHECK(val2 == "value1"); // Should return ref to existing value
        CHECK(*map.Find("key1") == "value1");

        // Modify through returned reference
        val2 = "modified_value";
        CHECK(*map.Find("key1") == "modified_value");
    }

    SUBCASE("First, Last, Bounds")
    {
        Map<int, String> map = { { 10, "ten" }, { 20, "twenty" }, { 30, "thirty" } };

        CHECK(map.First().HasValue());
        CHECK(map.First()->first == 10);
        CHECK(map.First()->second == "ten");

        CHECK(map.Last().HasValue());
        CHECK(map.Last()->first == 30);
        CHECK(map.Last()->second == "thirty");

        auto lower = map.LowerBoundEntry(20);
        CHECK(lower.HasValue());
        CHECK(lower->first == 20);

        auto lower2 = map.LowerBoundEntry(21);
        CHECK(lower2.HasValue());
        CHECK(lower2->first == 30);

        auto upper = map.UpperBoundEntry(20);
        CHECK(upper.HasValue());
        CHECK(upper->first == 30);

        auto upper2 = map.UpperBoundEntry(30);
        CHECK_FALSE(upper2.HasValue());

        Map<int, String> empty_map;
        CHECK_FALSE(empty_map.First().HasValue());
        CHECK_FALSE(empty_map.Last().HasValue());
        CHECK_FALSE(empty_map.LowerBoundEntry(1).HasValue());
        CHECK_FALSE(empty_map.UpperBoundEntry(1).HasValue());
    }
}

TEST_CASE("LinkedList API")
{
    SUBCASE("Default Construction")
    {
        LinkedList<int> list;
        CHECK(list.IsEmpty());
        CHECK(list.Len() == 0);
    }

    SUBCASE("Construction with count")
    {
        LinkedList<int> list(5);
        CHECK(list.Len() == 5);
        CHECK_FALSE(list.IsEmpty());
        CHECK(*list.Front() == 0); // Default constructed
        CHECK(*list.Back() == 0);
    }

    SUBCASE("Construction with count and value")
    {
        LinkedList<int> list(3, 10);
        CHECK(list.Len() == 3);
        CHECK(*list.Front() == 10);
        CHECK(*list.Back() == 10);
        auto it = list.begin();
        CHECK(*it == 10);
        ++it;
        CHECK(*it == 10);
    }

    SUBCASE("Initializer List Construction")
    {
        LinkedList<int> list = { 1, 2, 3, 4, 5 };
        CHECK(list.Len() == 5);
        CHECK(*list.Front() == 1);
        CHECK(*list.Back() == 5);
    }

    SUBCASE("Construction from iterators and range")
    {
        std::vector<int> vec = { 10, 20, 30 };
        LinkedList<int> list_from_iter(vec.begin(), vec.end());
        CHECK(list_from_iter.Len() == 3);
        CHECK(*list_from_iter.Front() == 10);
        CHECK(*list_from_iter.Back() == 30);

        LinkedList<int> list_from_range = LinkedList<int>::FromRange(vec);
        CHECK(list_from_range.Len() == 3);
        CHECK(*list_from_range.Front() == 10);
        CHECK(*list_from_range.Back() == 30);
    }

    SUBCASE("Operations on Empty List")
    {
        LinkedList<int> list;
        auto it = list.begin();
        CHECK(it == list.end());

        // 비어있는 리스트에 Remove(value) 또는 RemoveIf 호출
        CHECK(list.Remove(10) == 0);
        CHECK(list.RemoveIf([](int i){ return i > 0; }) == 0);
        CHECK(list.IsEmpty());

        // 비어있는 리스트에 Find 호출
        CHECK(list.Find(10) == list.end());
    }

    SUBCASE("Self-Assignment")
    {
        LinkedList<int> list = { 1, 2, 3 };
        // ReSharper disable once CppIdenticalOperandsInBinaryExpression
        list = list; // Self-assignment
        CHECK(list.Len() == 3);
        CHECK(*list.Front() == 1);

        LinkedList<int> list2 = { 4, 5, 6 };
        LinkedList<int>& ref = list2;
        list2 = ref;
        CHECK(list2.Len() == 3);
        CHECK(*list2.Front() == 4);
    }

    SUBCASE("Storing Pointers")
    {
        LinkedList<int*> list;
        int a = 1, b = 2;
        list.PushBack(&a);
        list.PushBack(nullptr);
        list.PushBack(&b);

        CHECK(list.Len() == 3);
        CHECK(*list.Find(nullptr) == nullptr);
        CHECK(list.Remove(nullptr) == 1);
        CHECK(list.Len() == 2);
    }

    SUBCASE("PushFront and PushBack")
    {
        LinkedList<int> list;
        list.PushBack(1);  // {1}
        list.PushFront(0); // {0, 1}
        list.PushBack(2);  // {0, 1, 2}
        CHECK(list.Len() == 3);
        CHECK(*list.Front() == 0);
        CHECK(*list.Back() == 2);

        auto it = list.begin();
        CHECK(*it == 0);
        ++it;
        CHECK(*it == 1);
        ++it;
        CHECK(*it == 2);
    }

    SUBCASE("PopFront and PopBack")
    {
        LinkedList<int> list = { 0, 1, 2 };
        auto val = list.PopFront();
        CHECK(val.HasValue());
        CHECK(*val == 0);
        CHECK(list.Len() == 2);
        CHECK(*list.Front() == 1);

        val = list.PopBack();
        CHECK(val.HasValue());
        CHECK(*val == 2);
        CHECK(list.Len() == 1);
        CHECK(*list.Back() == 1);

        list.PopFront();
        CHECK(list.IsEmpty());
        CHECK_FALSE(list.PopFront().HasValue());
        CHECK_FALSE(list.PopBack().HasValue());
    }

    SUBCASE("EmplaceFront and EmplaceBack")
    {
        struct TestStruct
        {
            int x;
            double y;

            TestStruct(int x, double y)
                : x(x)
                , y(y)
            {
            }

            bool operator==(const TestStruct& other) const { return x == other.x && y == other.y; }
        };

        LinkedList<TestStruct> list;
        list.EmplaceBack(1, 1.1);  // {{1, 1.1}}
        list.EmplaceFront(0, 0.0); // {{0, 0.0}, {1, 1.1}}
        CHECK(list.Len() == 2);
        CHECK(*list.Front() == TestStruct(0, 0.0));
        CHECK(*list.Back() == TestStruct(1, 1.1));
    }

    SUBCASE("Insert and Emplace")
    {
        LinkedList<int> list = { 1, 4 };
        auto it = list.begin();
        ++it; // points to 4

        list.Insert(it, 2); // {1, 2, 4}
        CHECK(list.Len() == 3);
        CHECK(*list.begin() == 1);
        CHECK(*std::next(list.begin()) == 2);
        CHECK(*list.Back() == 4);

        it = list.begin();
        std::advance(it, 2); // points to 4
        list.Emplace(it, 3); // {1, 2, 3, 4}
        CHECK(list.Len() == 4);
        CHECK(*std::next(list.begin(), 2) == 3);
    }

    SUBCASE("Remove by Iterator")
    {
        LinkedList<int> list = { 1, 2, 3, 4 };
        auto it = list.begin();
        ++it; // points to 2

        it = list.Remove(it); // {1, 3, 4}, it now points to 3
        CHECK(list.Len() == 3);
        CHECK(*list.begin() == 1);
        CHECK(*it == 3);

        list.Remove(list.begin()); // {3, 4}
        CHECK(list.Len() == 2);
        CHECK(*list.Front() == 3);

        list.Remove(std::next(list.begin())); // {3}
        CHECK(list.Len() == 1);
        CHECK(*list.Front() == 3);

        list.Remove(list.begin()); // {}
        CHECK(list.IsEmpty());
    }

    SUBCASE("Remove by Value")
    {
        LinkedList<int> list = { 1, 2, 1, 3, 1 };
        usize removed_count = list.Remove(1);
        CHECK(removed_count == 3);
        CHECK(list.Len() == 2);
        CHECK(*list.Front() == 2);
        CHECK(*list.Back() == 3);
    }

    SUBCASE("RemoveIf")
    {
        LinkedList<int> list = { 1, 2, 3, 4, 5, 6 };
        usize removed_count = list.RemoveIf([](int val) { return val % 2 == 0; }); // Remove even numbers
        CHECK(removed_count == 3);
        CHECK(list.Len() == 3);
        CHECK(*list.Front() == 1);
        CHECK(*std::next(list.begin()) == 3);
        CHECK(*list.Back() == 5);
    }

    SUBCASE("Clear")
    {
        LinkedList<int> list = { 1, 2, 3 };
        list.Clear();
        CHECK(list.IsEmpty());
        CHECK(list.Len() == 0);
        CHECK_FALSE(list.Front().HasValue());
    }

    SUBCASE("Find")
    {
        LinkedList<int> list = { 10, 20, 30, 20 };
        auto it = list.Find(20);
        CHECK(it != list.end());
        CHECK(*it == 20);
        ++it;
        CHECK(*it == 30); // Find returns first occurrence

        it = list.Find(50);
        CHECK(it == list.end());
    }

    SUBCASE("Iterators and Range-based for loop")
    {
        LinkedList<int> list = { 1, 2, 3, 4 };
        int sum = 0;
        for (const int val : list)
        {
            sum += val;
        }
        CHECK(sum == 10);

        // Test const iterators
        const LinkedList<int> const_list = { 5, 6 };
        int const_sum = 0;
        for (const int val : const_list)
        {
            const_sum += val;
        }
        CHECK(const_sum == 11);
    }

    SUBCASE("Iterator Invalidation Rules")
    {
        LinkedList<int> list = { 10, 20, 30, 40 };
        auto it20 = list.Find(20);
        auto it40 = list.Find(40);

        // PushFront/Back은 기존 이터레이터를 무효화하지 않음
        list.PushFront(0);
        list.PushBack(50);
        CHECK(*it20 == 20);
        CHECK(*it40 == 40);

        // Insert는 기존 이터레이터를 무효화하지 않음
        list.Insert(it20, 15);
        CHECK(*it20 == 20);
        CHECK(*it40 == 40);

        // Remove는 제거된 요소의 이터레이터만 무효화함
        auto it30 = list.Find(30);
        list.Remove(it30); // it30은 이제 무효화됨
        CHECK(*it20 == 20);
        CHECK(*it40 == 40);
    }
    SUBCASE("Const Correctness")
    {
        LinkedList<int> list = { 1, 2, 3 };
        const LinkedList<int>& const_list = list;

        CHECK(const_list.Len() == 3);
        CHECK(*const_list.Front() == 1);
        CHECK(*const_list.Back() == 3);

        // const_list.Find(...) 가 ConstIterator를 반환하는지 간접적으로 확인
        auto it = const_list.Find(2);
        CHECK(it != const_list.end());
        CHECK(*it == 2);

        // 컴파일 타임에 확인 (optional)
        static_assert(std::is_same_v<decltype(const_list.begin()), LinkedList<int>::ConstIterator>);
    }

    SUBCASE("Copy and Move Semantics")
    {
        LinkedList<int> list1 = { 1, 2, 3 };
        LinkedList<int> list2 = list1; // Copy constructor
        CHECK(list2.Len() == 3);
        CHECK(*list2.Front() == 1);
        CHECK(*list2.Back() == 3);

        LinkedList<int> list3;
        list3 = list1; // Copy assignment
        CHECK(list3.Len() == 3);
        CHECK(*list3.Front() == 1);

        LinkedList<int> list4 = std::move(list1); // Move constructor
        CHECK(list4.Len() == 3);
        CHECK(*list4.Front() == 1);
        CHECK(list1.IsEmpty()); // Moved-from state

        LinkedList<int> list5;
        list5 = std::move(list3); // Move assignment
        CHECK(list5.Len() == 3);
        CHECK(*list5.Front() == 1);
        CHECK(list3.IsEmpty());
    }
}

TEST_CASE("HashSet API")
{
    SUBCASE("Default Construction")
    {
        HashSet<int> set;
        CHECK(set.IsEmpty());
        CHECK(set.Len() == 0);
    }

    SUBCASE("Construction with capacity")
    {
        HashSet<int> set(10);
        CHECK(set.IsEmpty());
        CHECK(set.Len() == 0);
        CHECK(set.Capacity() >= 10);
    }

    SUBCASE("Initializer List Construction")
    {
        HashSet<int> set = { 1, 2, 3, 2, 1 };
        CHECK(set.Len() == 3);
        CHECK(set.Contains(1));
        CHECK(set.Contains(2));
        CHECK(set.Contains(3));
        CHECK_FALSE(set.Contains(4));
    }

    SUBCASE("Construction from iterators and range")
    {
        std::vector<int> vec = { 10, 20, 30, 20 };
        HashSet<int> set_from_iter(vec.begin(), vec.end());
        CHECK(set_from_iter.Len() == 3);
        CHECK(set_from_iter.Contains(10));
        CHECK(set_from_iter.Contains(20));
        CHECK(set_from_iter.Contains(30));

        HashSet<int> set_from_range = HashSet<int>::FromRange(vec);
        CHECK(set_from_range.Len() == 3);
        CHECK(set_from_range.Contains(10));
        CHECK(set_from_range.Contains(20));
        CHECK(set_from_range.Contains(30));
    }

    SUBCASE("Add and Contains")
    {
        HashSet<String> set;
        CHECK(set.Add("apple"));
        CHECK(set.Len() == 1);
        CHECK(set.Contains("apple"));
        CHECK_FALSE(set.Add("apple")); // Already exists
        CHECK(set.Len() == 1);

        set.Add("banana");
        CHECK(set.Len() == 2);
        CHECK(set.Contains("banana"));
    }

    SUBCASE("Emplace")
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
            size_t operator()(const TestStruct& ts) const
            {
                return std::hash<String>()(ts.name) ^ std::hash<int>()(ts.id);
            }
        };

        HashSet<TestStruct, TestStructHasher> set;
        CHECK(set.Emplace("item1", 1));
        CHECK(set.Len() == 1);
        CHECK(set.Contains(TestStruct("item1", 1)));

        CHECK_FALSE(set.Emplace("item1", 1)); // Already exists
        CHECK(set.Len() == 1);

        CHECK(set.Emplace("item2", 2));
        CHECK(set.Len() == 2);
    }

    SUBCASE("Remove")
    {
        HashSet<int> set = { 1, 2, 3 };
        CHECK(set.Remove(2));
        CHECK(set.Len() == 2);
        CHECK_FALSE(set.Contains(2));
        CHECK_FALSE(set.Remove(4)); // Not found
        CHECK(set.Len() == 2);
    }

    SUBCASE("RemoveIf")
    {
        HashSet<int> set = { 1, 2, 3, 4, 5, 6 };
        usize removed_count = set.RemoveIf([](int val) { return val % 2 == 0; }); // Remove even numbers
        CHECK(removed_count == 3);
        CHECK(set.Len() == 3);
        CHECK(set.Contains(1));
        CHECK(set.Contains(3));
        CHECK(set.Contains(5));
        CHECK_FALSE(set.Contains(2));
    }

    SUBCASE("Clear")
    {
        HashSet<int> set = { 1, 2, 3 };
        set.Clear();
        CHECK(set.IsEmpty());
        CHECK(set.Len() == 0);
        CHECK(set.Capacity() > 0); // Capacity is not necessarily 0 after clear
    }

    SUBCASE("Capacity and Reserve")
    {
        HashSet<int> set;
        set.Reserve(10);
        CHECK(set.Capacity() >= 10);
        set.Add(1);
        CHECK(set.Len() == 1);
    }

    SUBCASE("ToArray")
    {
        HashSet<int> set = { 3, 1, 2 };
        Array<int> arr = set.ToArray();
        CHECK(arr.Len() == 3);
        // Order is not guaranteed, so check for presence
        CHECK(arr.Contains(1));
        CHECK(arr.Contains(2));
        CHECK(arr.Contains(3));
    }

    SUBCASE("Iterators and Range-based for loop")
    {
        HashSet<int> set = { 10, 20, 30 };
        int sum = 0;
        for (int val : set)
        {
            sum += val;
        }
        CHECK(sum == 60);

        const HashSet<int> const_set = { 40, 50 };
        int const_sum = 0;
        for (int val : const_set)
        {
            const_sum += val;
        }
        CHECK(const_sum == 90);
    }

    SUBCASE("Copy and Move Semantics")
    {
        HashSet<int> set1 = { 1, 2, 3 };
        HashSet<int> set2 = set1; // Copy constructor
        CHECK(set2.Len() == 3);
        CHECK(set2.Contains(1));

        HashSet<int> set3;
        set3 = set1; // Copy assignment
        CHECK(set3.Len() == 3);
        CHECK(set3.Contains(2));

        HashSet<int> set4 = std::move(set1); // Move constructor
        CHECK(set4.Len() == 3);
        CHECK(set4.Contains(3));
        CHECK(set1.IsEmpty()); // Moved-from state

        HashSet<int> set5;
        set5 = std::move(set3); // Move assignment
        CHECK(set5.Len() == 3);
        CHECK(set5.Contains(1));
        CHECK(set3.IsEmpty());
    }
}

TEST_CASE("Set API")
{
    SUBCASE("Default Construction")
    {
        Set<int> set;
        CHECK(set.IsEmpty());
        CHECK(set.Len() == 0);
    }

    SUBCASE("Initializer List Construction")
    {
        Set<int> set = { 3, 1, 2, 1, 3 };
        CHECK(set.Len() == 3);
        CHECK(set.Contains(1));
        CHECK(set.Contains(2));
        CHECK(set.Contains(3));
        CHECK_FALSE(set.Contains(4));
    }

    SUBCASE("Construction from iterators and range")
    {
        std::vector<int> vec = { 30, 10, 20, 10 };
        Set<int> set_from_iter(vec.begin(), vec.end());
        CHECK(set_from_iter.Len() == 3);
        CHECK(set_from_iter.Contains(10));
        CHECK(set_from_iter.Contains(20));
        CHECK(set_from_iter.Contains(30));

        Set<int> set_from_range = Set<int>::FromRange(vec);
        CHECK(set_from_range.Len() == 3);
        CHECK(set_from_range.Contains(10));
        CHECK(set_from_range.Contains(20));
        CHECK(set_from_range.Contains(30));
    }

    SUBCASE("Add and Contains")
    {
        Set<String> set;
        CHECK(set.Add("apple"));
        CHECK(set.Len() == 1);
        CHECK(set.Contains("apple"));
        CHECK_FALSE(set.Add("apple")); // Already exists
        CHECK(set.Len() == 1);

        set.Add("banana");
        CHECK(set.Len() == 2);
        CHECK(set.Contains("banana"));
    }

    SUBCASE("Emplace")
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
        CHECK(set.Emplace("item1", 1));
        CHECK(set.Len() == 1);
        CHECK(set.Contains(TestStruct("item1", 1)));

        CHECK_FALSE(set.Emplace("item1", 1)); // Already exists
        CHECK(set.Len() == 1);

        CHECK(set.Emplace("item2", 2));
        CHECK(set.Len() == 2);
    }

    SUBCASE("Remove")
    {
        Set<int> set = { 1, 2, 3 };
        CHECK(set.Remove(2));
        CHECK(set.Len() == 2);
        CHECK_FALSE(set.Contains(2));
        CHECK_FALSE(set.Remove(4)); // Not found
        CHECK(set.Len() == 2);
    }

    SUBCASE("RemoveIf")
    {
        Set<int> set = { 1, 2, 3, 4, 5, 6 };
        usize removed_count = set.RemoveIf([](int val) { return val % 2 == 0; }); // Remove even numbers
        CHECK(removed_count == 3);
        CHECK(set.Len() == 3);
        CHECK(set.Contains(1));
        CHECK(set.Contains(3));
        CHECK(set.Contains(5));
        CHECK_FALSE(set.Contains(2));
    }

    SUBCASE("Clear")
    {
        Set<int> set = { 1, 2, 3 };
        set.Clear();
        CHECK(set.IsEmpty());
        CHECK(set.Len() == 0);
    }

    SUBCASE("ToArray")
    {
        Set<int> set = { 3, 1, 2 };
        Array<int> arr = set.ToArray();
        CHECK(arr.Len() == 3);
        // Order is guaranteed for Set
        CHECK(arr[0] == 1);
        CHECK(arr[1] == 2);
        CHECK(arr[2] == 3);
    }

    SUBCASE("Iterators and Range-based for loop")
    {
        Set<int> set = { 30, 10, 20 };
        int sum = 0;
        String concat_str;
        for (int val : set) // Should iterate in order: 10, 20, 30
        {
            sum += val;
            concat_str.Append(String::Format("{}", val));
        }
        CHECK(sum == 60);
        CHECK(concat_str == "102030");

        const Set<int> const_set = { 50, 40 };
        int const_sum = 0;
        for (int val : const_set)
        {
            const_sum += val;
        }
        CHECK(const_sum == 90);
    }

    SUBCASE("Copy and Move Semantics")
    {
        Set<int> set1 = { 1, 2, 3 };
        Set<int> set2 = set1; // Copy constructor
        CHECK(set2.Len() == 3);
        CHECK(set2.Contains(1));

        Set<int> set3;
        set3 = set1; // Copy assignment
        CHECK(set3.Len() == 3);
        CHECK(set3.Contains(2));

        Set<int> set4 = std::move(set1); // Move constructor
        CHECK(set4.Len() == 3);
        CHECK(set4.Contains(3));
        CHECK(set1.IsEmpty()); // Moved-from state

        Set<int> set5;
        set5 = std::move(set3); // Move assignment
        CHECK(set5.Len() == 3);
        CHECK(set5.Contains(1));
        CHECK(set3.IsEmpty());
    }
}

TEST_CASE("PriorityQueue API")
{
    SUBCASE("Default Construction and Push/Pop (Max-Heap)")
    {
        PriorityQueue<int> pq;
        CHECK(pq.IsEmpty());
        CHECK(pq.Len() == 0);

        pq.Push(10); // {10}
        pq.Push(30); // {30, 10}
        pq.Push(20); // {30, 10, 20} (heap property)
        pq.Push(5);  // {30, 10, 20, 5} (heap property)

        CHECK_FALSE(pq.IsEmpty());
        CHECK(pq.Len() == 4);
        CHECK(*pq.Peek() == 30);

        CHECK(*pq.Pop() == 30); // {20, 10, 5}
        CHECK(pq.Len() == 3);
        CHECK(*pq.Peek() == 20);

        CHECK(*pq.Pop() == 20); // {10, 5}
        CHECK(*pq.Pop() == 10); // {5}
        CHECK(*pq.Pop() == 5);  // {}
        CHECK(pq.IsEmpty());
        CHECK_FALSE(pq.Pop().HasValue());
    }

    SUBCASE("Construction from Iterators")
    {
        std::vector<int> vec = { 10, 30, 20, 5 };
        PriorityQueue<int> pq(vec.begin(), vec.end());
        CHECK(pq.Len() == 4);
        CHECK(*pq.Peek() == 30); // Max element
    }

    SUBCASE("Emplace")
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

        CHECK(pq.Len() == 3);
        CHECK(pq.Peek()->priority == 30);
        CHECK(pq.Peek()->name == "high");

        CHECK(pq.Pop()->priority == 30);
        CHECK(pq.Pop()->priority == 20);
        CHECK(pq.Pop()->priority == 10);
    }

    SUBCASE("PushRange")
    {
        PriorityQueue<int> pq;
        std::vector<int> values = { 1, 5, 2, 8, 3 };
        pq.PushRange(values);
        CHECK(pq.Len() == 5);
        CHECK(*pq.Peek() == 8);
        CHECK(*pq.Pop() == 8);
        CHECK(*pq.Pop() == 5);
    }

    SUBCASE("Clear")
    {
        PriorityQueue<int> pq = { 1, 2, 3 };
        pq.Clear();
        CHECK(pq.IsEmpty());
        CHECK(pq.Len() == 0);
        CHECK_FALSE(pq.Peek().HasValue());
    }

    SUBCASE("ToUnderlyingContainer")
    {
        PriorityQueue<int> pq = { 10, 30, 20 };
        Array<int> arr = pq.ToUnderlyingContainer();
        CHECK(arr.Len() == 3);
        // The underlying container is a heap, not necessarily sorted
        // So we check if the elements are present
        CHECK(arr.Contains(10));
        CHECK(arr.Contains(20));
        CHECK(arr.Contains(30));
    }

    SUBCASE("Min-Heap (Custom Compare)")
    {
        PriorityQueue<int, Array<int>, std::greater<>> min_pq; // Use std::greater for min-heap
        min_pq.Push(10);
        min_pq.Push(30);
        min_pq.Push(20);
        min_pq.Push(5);

        CHECK(*min_pq.Peek() == 5);
        CHECK(*min_pq.Pop() == 5);
        CHECK(*min_pq.Pop() == 10);
        CHECK(*min_pq.Pop() == 20);
        CHECK(*min_pq.Pop() == 30);
        CHECK(min_pq.IsEmpty());
    }

    SUBCASE("Swap")
    {
        PriorityQueue<int> pq1 = { 1, 5, 2 };
        PriorityQueue<int> pq2 = { 10, 30, 20 };

        pq1.Swap(pq2);

        CHECK(pq1.Len() == 3);
        CHECK(*pq1.Peek() == 30);

        CHECK(pq2.Len() == 3);
        CHECK(*pq2.Peek() == 5);
    }
}
}
