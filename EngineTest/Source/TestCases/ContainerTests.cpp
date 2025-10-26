#include "doctest/doctest.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FixedArray.h"

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

        FixedArray<int, 0> empty_arr;
        CHECK(empty_arr.Len() == 0);
        CHECK(empty_arr.IsEmpty());
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
        CHECK_FALSE(arr.At(-1).HasValue());
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
        CHECK(arr.IsEmpty());
        CHECK(arr.Len() == 0);
    }

    SUBCASE("Initializer List Construction")
    {
        Array<int> arr = { 1, 2, 3 };
        CHECK_FALSE(arr.IsEmpty());
        CHECK(arr.Len() == 3);
        CHECK(arr[0] == 1);
        CHECK(arr[1] == 2);
        CHECK(arr[2] == 3);
    }

    SUBCASE("Add and Emplace")
    {
        Array<int> arr;
        arr.Add(10);
        CHECK(arr.Len() == 1);
        CHECK(arr[0] == 10);

        arr.Emplace(20);
        CHECK(arr.Len() == 2);
        CHECK(arr[1] == 20);
    }

    SUBCASE("Accessors: At, [], Front, Back")
    {
        Array<std::string> arr = { "hello", "world", "!" };

        CHECK(arr.At(1).HasValue());
        CHECK(*arr.At(1) == "world");
        CHECK_FALSE(arr.At(3).HasValue());

        CHECK(arr[0] == "hello");

        CHECK(arr.Front().HasValue());
        CHECK(*arr.Front() == "hello");

        CHECK(arr.Back().HasValue());
        CHECK(*arr.Back() == "!");

        const auto& const_arr = arr;
        CHECK(*const_arr.At(1) == "world");
        CHECK(const_arr[0] == "hello");
        CHECK(*const_arr.Front() == "hello");
        CHECK(*const_arr.Back() == "!");
    }

    SUBCASE("Pop")
    {
        Array<int> arr = { 1, 2, 3 };
        auto popped = arr.Pop();
        CHECK(popped.HasValue());
        CHECK(*popped == 3);
        CHECK(arr.Len() == 2);
        CHECK(*arr.Back() == 2);

        arr.Pop();
        arr.Pop();
        CHECK(arr.IsEmpty());
        auto empty_pop = arr.Pop();
        CHECK_FALSE(empty_pop.HasValue());
    }

    SUBCASE("RemoveAtSwap")
    {
        Array<int> arr = { 10, 20, 30, 40, 50 };

        // Remove middle element
        bool removed = arr.RemoveAtSwap(2); // remove 30
        CHECK(removed);
        CHECK(arr.Len() == 4);
        CHECK(arr.Contains(50)); // 30 should be replaced by 50
        CHECK_FALSE(arr.Contains(30));

        // Remove last element
        removed = arr.RemoveAtSwap(arr.Len() - 1);
        CHECK(removed);
        CHECK(arr.Len() == 3);

        // Remove out of bounds
        removed = arr.RemoveAtSwap(10);
        CHECK_FALSE(removed);
    }

    SUBCASE("Find and Contains")
    {
        Array<int> arr = { 10, 20, 30, 20 };
        CHECK(arr.Contains(20));
        CHECK_FALSE(arr.Contains(99));

        auto found_index = arr.Find(20);
        CHECK(found_index.HasValue());
        CHECK(*found_index == 1);

        auto not_found_index = arr.Find(99);
        CHECK_FALSE(not_found_index.HasValue());
    }

    SUBCASE("Capacity and Memory Management")
    {
        Array<int> arr;
        CHECK(arr.Capacity() == 0);

        arr.Reserve(10);
        CHECK(arr.Capacity() >= 10);
        auto old_capacity = arr.Capacity();

        arr.Add(1);
        arr.Add(2);
        CHECK(arr.Capacity() == old_capacity);

        arr.ShrinkToFit();
        CHECK(arr.Capacity() == arr.Len());
        CHECK(arr.Capacity() == 2);
    }

    SUBCASE("Resize and Clear")
    {
        Array<int> arr = { 1, 2, 3 };
        arr.Resize(5);
        CHECK(arr.Len() == 5);
        // New elements are default-initialized (to 0 for int)
        CHECK(arr[3] == 0);
        CHECK(arr[4] == 0);

        arr.Resize(2);
        CHECK(arr.Len() == 2);
        CHECK(*arr.Back() == 2);

        arr.Clear();
        CHECK(arr.IsEmpty());
        CHECK(arr.Len() == 0);
        CHECK(arr.Capacity() > 0); // Clear doesn't change capacity
    }

    SUBCASE("Copy and Move Semantics")
    {
        Array<int> arr1 = { 1, 2, 3 };

        // Copy constructor
        Array<int> arr2 = arr1;
        CHECK(arr2.Len() == 3);
        CHECK(arr1.Len() == 3);
        CHECK(arr2[1] == 2);

        // Copy assignment
        Array<int> arr3;
        arr3 = arr1;
        CHECK(arr3.Len() == 3);
        CHECK(arr3[1] == 2);

        // Move constructor
        Array<int> arr4 = std::move(arr1);
        CHECK(arr4.Len() == 3);
        CHECK(arr4[1] == 2);
        // arr1 is in a valid but unspecified state, but should be empty
        CHECK(arr1.IsEmpty());

        // Move assignment
        Array<int> arr5;
        arr5 = std::move(arr2);
        CHECK(arr5.Len() == 3);
        CHECK(arr5[1] == 2);
        CHECK(arr2.IsEmpty());
    }

    SUBCASE("Range-based for loop")
    {
        Array<int> arr = { 10, 20, 30 };
        int sum = 0;
        for (const auto& val : arr)
        {
            sum += val;
        }
        CHECK(sum == 60);
    }
}
}
