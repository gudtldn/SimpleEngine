#include "doctest/doctest.h"
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
}
