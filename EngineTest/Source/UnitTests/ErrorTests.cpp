#include "gtest/gtest.h"

#include <cstring>
#include <memory>
#include <string>

#include "SimpleEngine/Core/Container/String.h" // For error messages
#include "SimpleEngine/Core/Error/Expected.h"

using namespace se;

class ExpectedAPI_Test : public ::testing::Test {};


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


TEST_F(ExpectedAPI_Test, ConstructionWithValue)
{
    Expected<int, TestError> e1(10);
    EXPECT_TRUE(e1.HasValue());
    EXPECT_FALSE(e1.HasError());
    EXPECT_EQ(e1.Value(), 10);

    Expected<String, TestError> e2("hello");
    EXPECT_EQ(e2.Value(), "hello");

    // In-place construction
    Expected<TestValue, TestError> e3(std::in_place, 42);
    EXPECT_TRUE(e3.HasValue());
    EXPECT_EQ(e3.Value().value, 42);
}

TEST_F(ExpectedAPI_Test, ConstructionWithError)
{
    Expected<int, TestError> e1{ Unexpected(TestError::DefaultError) };
    EXPECT_FALSE(e1.HasValue());
    EXPECT_TRUE(e1.HasError());
    EXPECT_EQ(e1.Error(), TestError::DefaultError);

    Expected<int, String> e2(Unexpected("error message"));
    EXPECT_EQ(e2.Error(), "error message");
}

TEST_F(ExpectedAPI_Test, CopyAndMoveSemantics)
{
    // Copy
    Expected<int, TestError> e1(10);
    Expected<int, TestError> e2 = e1;
    EXPECT_TRUE(e2.HasValue());
    EXPECT_EQ(e2.Value(), 10);

    Expected<int, TestError> e3{ Unexpected(TestError::DefaultError) };
    Expected<int, TestError> e4 = e3;
    EXPECT_TRUE(e4.HasError());
    EXPECT_EQ(e4.Error(), TestError::DefaultError);

    // Move
    Expected<std::unique_ptr<int>, TestError> e5(std::make_unique<int>(20));
    Expected<std::unique_ptr<int>, TestError> e6 = std::move(e5);
    EXPECT_TRUE(e6.HasValue());
    EXPECT_EQ(*e6.Value(), 20);
    EXPECT_EQ(e5.Value(), nullptr); // Moved from

    Expected<int, std::unique_ptr<String>> e7(Unexpected(std::make_unique<String>("error")));
    Expected<int, std::unique_ptr<String>> e8 = std::move(e7);
    EXPECT_TRUE(e8.HasError());
    EXPECT_EQ(*e8.Error(), "error");
    EXPECT_EQ(e7.Error(), nullptr); // Moved from
}

TEST_F(ExpectedAPI_Test, Accessors)
{
    Expected<int, TestError> e_val(10);
    EXPECT_EQ(*e_val, 10);
    EXPECT_EQ(e_val.Value(), 10);

    const Expected<int, TestError> ce_val(20);
    EXPECT_EQ(*ce_val, 20);
    EXPECT_EQ(ce_val.Value(), 20);

    Expected<TestValue, TestError> e_struct(std::in_place, 30);
    EXPECT_EQ(e_struct->value, 30);

    const Expected<TestValue, TestError> ce_struct(std::in_place, 40);
    EXPECT_EQ(ce_struct->value, 40);

    Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    EXPECT_EQ(e_err.Error(), TestError::DefaultError);
}

TEST_F(ExpectedAPI_Test, ValueOr)
{
    Expected<int, TestError> e_val(10);
    EXPECT_EQ(e_val.ValueOr(20), 10);

    Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    EXPECT_EQ(e_err.ValueOr(20), 20);

    // Test r-value overload
    EXPECT_EQ(std::move(e_val).ValueOr(30), 10);
    EXPECT_EQ(std::move(e_err).ValueOr(30), 30);
}

TEST_F(ExpectedAPI_Test, Map)
{
    auto mapper = [](int i) { return std::to_string(i); };

    Expected<int, TestError> e_val(42);
    auto mapped_val = e_val.Map(mapper);
    EXPECT_TRUE(mapped_val.HasValue());
    EXPECT_EQ(mapped_val.Value(), "42");

    Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    auto mapped_err = e_err.Map(mapper);
    EXPECT_TRUE(mapped_err.HasError());
    EXPECT_EQ(mapped_err.Error(), TestError::DefaultError);
}

TEST_F(ExpectedAPI_Test, MapError)
{
    auto error_mapper = [](TestError err)
    {
        return (err == TestError::DefaultError) ? "Default" : "Other";
    };

    Expected<int, TestError> e_val(10);
    auto mapped_val = e_val.MapError(error_mapper);
    EXPECT_TRUE(mapped_val.HasValue());
    EXPECT_EQ(mapped_val.Value(), 10);

    Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    auto mapped_err = e_err.MapError(error_mapper);
    EXPECT_TRUE(mapped_err.HasError());
    EXPECT_EQ(std::strcmp(mapped_err.Error(), "Default"), 0);
}

TEST_F(ExpectedAPI_Test, AndThen)
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
    EXPECT_TRUE(res1.HasValue());
    EXPECT_EQ(res1.Value(), "10");

    auto res2 = e_val.AndThen(then_func_err);
    EXPECT_TRUE(res2.HasError());
    EXPECT_EQ(res2.Error(), TestError::AnotherError);

    Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    auto res3 = e_err.AndThen(then_func_ok);
    EXPECT_TRUE(res3.HasError());
    EXPECT_EQ(res3.Error(), TestError::DefaultError);
}

TEST_F(ExpectedAPI_Test, OrElse)
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
    EXPECT_TRUE(res1.HasValue());
    EXPECT_EQ(res1.Value(), 10);

    Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    auto res2 = e_err.OrElse(else_func_ok);
    EXPECT_TRUE(res2.HasValue());
    EXPECT_EQ(res2.Value(), 100);

    auto res3 = e_err.OrElse(else_func_err);
    EXPECT_TRUE(res3.HasError());
    EXPECT_EQ(res3.Error(), "Recovered error");
}
