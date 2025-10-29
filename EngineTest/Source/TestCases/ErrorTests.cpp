#include "doctest/doctest.h"
#include <string>
#include <memory>

#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Container/String.h" // For error messages

using namespace se;


enum class TestError
{
    DefaultError,
    AnotherError,
};

// A simple struct for testing non-trivial types
struct TestValue
{
    int value;
    bool operator==(const TestValue& other) const { return value == other.value; }
};

TEST_SUITE("SimpleEngine Core Error API")
{
TEST_CASE("Expected API")
{
    SUBCASE("Construction with Value")
    {
        Expected<int, TestError> e1(10);
        CHECK(e1.HasValue());
        CHECK_FALSE(e1.HasError());
        CHECK(e1.Value() == 10);

        Expected<String, TestError> e2("hello");
        CHECK(e2.Value() == "hello");

        // In-place construction
        Expected<TestValue, TestError> e3(std::in_place, 42);
        CHECK(e3.HasValue());
        CHECK(e3.Value().value == 42);
    }

    SUBCASE("Construction with Error")
    {
        Expected<int, TestError> e1{ Unexpected(TestError::DefaultError) };
        CHECK_FALSE(e1.HasValue());
        CHECK(e1.HasError());
        CHECK(e1.Error() == TestError::DefaultError);

        Expected<int, String> e2(Unexpected("error message"));
        CHECK(e2.Error() == "error message");
    }

    SUBCASE("Copy and Move Semantics")
    {
        // Copy
        Expected<int, TestError> e1(10);
        Expected<int, TestError> e2 = e1;
        CHECK(e2.HasValue());
        CHECK(e2.Value() == 10);

        Expected<int, TestError> e3{ Unexpected(TestError::DefaultError) };
        Expected<int, TestError> e4 = e3;
        CHECK(e4.HasError());
        CHECK(e4.Error() == TestError::DefaultError);

        // Move
        Expected<std::unique_ptr<int>, TestError> e5(std::make_unique<int>(20));
        Expected<std::unique_ptr<int>, TestError> e6 = std::move(e5);
        CHECK(e6.HasValue());
        CHECK(*e6.Value() == 20);
        CHECK(e5.Value() == nullptr); // Moved from

        Expected<int, std::unique_ptr<String>> e7(Unexpected(std::make_unique<String>("error")));
        Expected<int, std::unique_ptr<String>> e8 = std::move(e7);
        CHECK(e8.HasError());
        CHECK(*e8.Error() == "error");
        CHECK(e7.Error() == nullptr); // Moved from
    }

    SUBCASE("Accessors")
    {
        Expected<int, TestError> e_val(10);
        CHECK(*e_val == 10);
        CHECK(e_val.Value() == 10);

        const Expected<int, TestError> ce_val(20);
        CHECK(*ce_val == 20);
        CHECK(ce_val.Value() == 20);

        Expected<TestValue, TestError> e_struct(std::in_place, 30);
        CHECK(e_struct->value == 30);

        const Expected<TestValue, TestError> ce_struct(std::in_place, 40);
        CHECK(ce_struct->value == 40);

        Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
        CHECK(e_err.Error() == TestError::DefaultError);
    }

    SUBCASE("ValueOr")
    {
        Expected<int, TestError> e_val(10);
        CHECK(e_val.ValueOr(20) == 10);

        Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
        CHECK(e_err.ValueOr(20) == 20);

        // Test r-value overload
        CHECK(std::move(e_val).ValueOr(30) == 10);
        CHECK(std::move(e_err).ValueOr(30) == 30);
    }

    SUBCASE("Map")
    {
        auto mapper = [](int i) { return std::to_string(i); };

        Expected<int, TestError> e_val(42);
        auto mapped_val = e_val.Map(mapper);
        CHECK(mapped_val.HasValue());
        CHECK(mapped_val.Value() == "42");

        Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
        auto mapped_err = e_err.Map(mapper);
        CHECK(mapped_err.HasError());
        CHECK(mapped_err.Error() == TestError::DefaultError);
    }

    SUBCASE("MapError")
    {
        auto error_mapper = [](TestError err)
        {
            return (err == TestError::DefaultError) ? "Default" : "Other";
        };

        Expected<int, TestError> e_val(10);
        auto mapped_val = e_val.MapError(error_mapper);
        CHECK(mapped_val.HasValue());
        CHECK(mapped_val.Value() == 10);

        Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
        auto mapped_err = e_err.MapError(error_mapper);
        CHECK(mapped_err.HasError());
        CHECK(std::strcmp(mapped_err.Error(), "Default") == 0);
    }

    SUBCASE("AndThen")
    {
        auto then_func_ok = [](int i) -> Expected<String, TestError>
        {
            return std::to_string(i);
        };
        auto then_func_err = []([[maybe_unused]] int i) -> Expected<String, TestError>
        {
            return Unexpected(TestError::AnotherError);
        };

        Expected<int, TestError> e_val(10);
        auto res1 = e_val.AndThen(then_func_ok);
        CHECK(res1.HasValue());
        CHECK(res1.Value() == "10");

        auto res2 = e_val.AndThen(then_func_err);
        CHECK(res2.HasError());
        CHECK(res2.Error() == TestError::AnotherError);

        Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
        auto res3 = e_err.AndThen(then_func_ok);
        CHECK(res3.HasError());
        CHECK(res3.Error() == TestError::DefaultError);
    }

    SUBCASE("OrElse")
    {
        auto else_func_ok = []([[maybe_unused]] TestError e) -> Expected<int, String>
        {
            return 100;
        };
        auto else_func_err = []([[maybe_unused]] TestError e) -> Expected<int, String>
        {
            return Unexpected("Recovered error");
        };

        Expected<int, TestError> e_val(10);
        auto res1 = e_val.OrElse(else_func_ok);
        CHECK(res1.HasValue());
        CHECK(res1.Value() == 10);

        Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
        auto res2 = e_err.OrElse(else_func_ok);
        CHECK(res2.HasValue());
        CHECK(res2.Value() == 100);

        auto res3 = e_err.OrElse(else_func_err);
        CHECK(res3.HasError());
        CHECK(res3.Error() == "Recovered error");
    }
}
}
