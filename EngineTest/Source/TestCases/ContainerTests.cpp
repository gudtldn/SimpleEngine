#include "doctest/doctest.h"
#include <iostream>
#include <vector>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
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

        Array<int> arr_from_range(vec);
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
        arr2.Insert(1, to_insert);
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

    SUBCASE("Append")
    {
        Array<int> arr1 = { 1, 2 };
        Array<int> arr2 = { 3, 4 };
        arr1.Append(arr2);
        CHECK(arr1.Len() == 4);
        CHECK(arr1[2] == 3);
        CHECK(arr1[3] == 4);

        std::vector<int> vec = { 5, 6 };
        arr1.Append(vec);
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

    SUBCASE("Append and Concatenation")
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
}
