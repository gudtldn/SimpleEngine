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
        return std::to_string(i).data();
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

// ============================================================================
// Constexpr Tests
// ============================================================================

TEST_F(ExpectedAPI_Test, ConstexprConstruction)
{
    // constexpr 값 생성
    constexpr Expected<int, TestError> e_val(42);
    static_assert(e_val.HasValue());
    static_assert(!e_val.HasError());
    static_assert(e_val.Value() == 42);

    // constexpr 에러 생성
    constexpr Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    static_assert(!e_err.HasValue());
    static_assert(e_err.HasError());
    static_assert(e_err.Error() == TestError::DefaultError);
}

TEST_F(ExpectedAPI_Test, ConstexprValueOr)
{
    constexpr Expected<int, TestError> e_val(42);
    static_assert(e_val.ValueOr(99) == 42);

    constexpr Expected<int, TestError> e_err{ Unexpected(TestError::DefaultError) };
    static_assert(e_err.ValueOr(99) == 99);
}

TEST_F(ExpectedAPI_Test, ConstexprMapAndAndThen)
{
    // constexpr Map
    constexpr auto map_result = []() constexpr {
        Expected<int, TestError> e(21);
        return e.Map([](int n) { return n * 2; });
    }();
    static_assert(map_result.HasValue());
    static_assert(map_result.Value() == 42);

    // constexpr AndThen
    constexpr auto and_then_result = []() constexpr {
        Expected<int, TestError> e(10);
        return e.AndThen([](int n) -> Expected<int, TestError> {
            if (n > 0) return n * 2;
            return Unexpected(TestError::AnotherError);
        });
    }();
    static_assert(and_then_result.HasValue());
    static_assert(and_then_result.Value() == 20);

    // constexpr OrElse
    constexpr auto or_else_result = []() constexpr {
        Expected<int, TestError> e{ Unexpected(TestError::DefaultError) };
        return e.OrElse([](TestError) -> Expected<int, TestError> { return 99; });
    }();
    static_assert(or_else_result.HasValue());
    static_assert(or_else_result.Value() == 99);

    // constexpr MapError
    constexpr auto map_error_result = []() constexpr {
        Expected<int, TestError> e{ Unexpected(TestError::DefaultError) };
        return e.MapError([](TestError err) { return static_cast<int>(err); });
    }();
    static_assert(map_error_result.HasError());
    static_assert(map_error_result.Error() == 0); // DefaultError = 0
}

TEST_F(ExpectedAPI_Test, DeducingThisValueCategories)
{
    // lvalue Value access
    {
        Expected<std::string, TestError> e("test");
        std::string& val = e.Value();
        val = "modified";
        EXPECT_EQ(e.Value(), "modified");
    }

    // const lvalue Value access
    {
        const Expected<std::string, TestError> e("test");
        const std::string& val = e.Value();
        EXPECT_EQ(val, "test");
    }

    // rvalue Value access
    {
        Expected<std::string, TestError> e("test");
        std::string val = std::move(e).Value();
        EXPECT_EQ(val, "test");
    }

    // lvalue Error access
    {
        Expected<int, std::string> e{ Unexpected("error") };
        std::string& err = e.Error();
        err = "modified error";
        EXPECT_EQ(e.Error(), "modified error");
    }

    // rvalue Error access
    {
        Expected<int, std::string> e{ Unexpected("error") };
        std::string err = std::move(e).Error();
        EXPECT_EQ(err, "error");
    }

    // Map with different value categories
    {
        Expected<int, TestError> e(10);

        // lvalue
        auto res1 = e.Map([](int& n) { return n * 2; });
        EXPECT_EQ(res1.Value(), 20);

        // rvalue
        auto res2 = std::move(e).Map([](int&& n) { return n * 3; });
        EXPECT_EQ(res2.Value(), 30);
    }
}

// ============================================================================
// Expected<void, E> Tests
// ============================================================================

TEST_F(ExpectedAPI_Test, ExpectedVoidConstruction)
{
    // 성공 상태 생성
    Expected<void, TestError> e_success;
    EXPECT_TRUE(e_success.HasValue());
    EXPECT_FALSE(e_success.HasError());

    // 에러 상태 생성
    Expected<void, TestError> e_error{ Unexpected(TestError::DefaultError) };
    EXPECT_FALSE(e_error.HasValue());
    EXPECT_TRUE(e_error.HasError());
    EXPECT_EQ(e_error.Error(), TestError::DefaultError);
}

TEST_F(ExpectedAPI_Test, ExpectedVoidCopyAndMove)
{
    // Copy
    Expected<void, TestError> e1;
    Expected<void, TestError> e2 = e1;
    EXPECT_TRUE(e2.HasValue());

    Expected<void, TestError> e3{ Unexpected(TestError::DefaultError) };
    Expected<void, TestError> e4 = e3;
    EXPECT_TRUE(e4.HasError());
    EXPECT_EQ(e4.Error(), TestError::DefaultError);

    // Move
    Expected<void, std::unique_ptr<String>> e5(Unexpected(std::make_unique<String>("error")));
    Expected<void, std::unique_ptr<String>> e6 = std::move(e5);
    EXPECT_TRUE(e6.HasError());
    EXPECT_EQ(*e6.Error(), "error");
}

TEST_F(ExpectedAPI_Test, ExpectedVoidEmplace)
{
    Expected<void, TestError> e{ Unexpected(TestError::DefaultError) };
    EXPECT_TRUE(e.HasError());

    e.Emplace();
    EXPECT_TRUE(e.HasValue());
    EXPECT_FALSE(e.HasError());
}

TEST_F(ExpectedAPI_Test, ExpectedVoidAndThen)
{
    auto success_then = []() -> Expected<int, TestError> { return 42; };
    auto error_then = []() -> Expected<int, TestError> { return Unexpected(TestError::AnotherError); };

    Expected<void, TestError> e_success;
    auto res1 = e_success.AndThen(success_then);
    EXPECT_TRUE(res1.HasValue());
    EXPECT_EQ(res1.Value(), 42);

    auto res2 = e_success.AndThen(error_then);
    EXPECT_TRUE(res2.HasError());
    EXPECT_EQ(res2.Error(), TestError::AnotherError);

    Expected<void, TestError> e_error{ Unexpected(TestError::DefaultError) };
    auto res3 = e_error.AndThen(success_then);
    EXPECT_TRUE(res3.HasError());
    EXPECT_EQ(res3.Error(), TestError::DefaultError);
}

TEST_F(ExpectedAPI_Test, ExpectedVoidMap)
{
    // void -> T
    Expected<void, TestError> e_success;
    auto res1 = e_success.Map([]() { return 42; });
    EXPECT_TRUE(res1.HasValue());
    EXPECT_EQ(res1.Value(), 42);

    // void -> void
    bool called = false;
    auto res2 = e_success.Map([&called]() { called = true; });
    EXPECT_TRUE(res2.HasValue());
    EXPECT_TRUE(called);

    // 에러 상태에서는 함수 호출 안됨
    Expected<void, TestError> e_error{ Unexpected(TestError::DefaultError) };
    bool error_called = false;
    auto res3 = e_error.Map([&error_called]() { error_called = true; return 42; });
    EXPECT_TRUE(res3.HasError());
    EXPECT_FALSE(error_called);
}

TEST_F(ExpectedAPI_Test, ExpectedVoidOrElse)
{
    auto recover = [](TestError) -> Expected<void, String> { return {}; };
    auto fail_recover = [](TestError) -> Expected<void, String> { return Unexpected(String("recovered")); };

    Expected<void, TestError> e_success;
    auto res1 = e_success.OrElse(recover);
    EXPECT_TRUE(res1.HasValue());

    Expected<void, TestError> e_error{ Unexpected(TestError::DefaultError) };
    auto res2 = e_error.OrElse(recover);
    EXPECT_TRUE(res2.HasValue());

    auto res3 = e_error.OrElse(fail_recover);
    EXPECT_TRUE(res3.HasError());
    EXPECT_EQ(res3.Error(), "recovered");
}

TEST_F(ExpectedAPI_Test, ExpectedVoidMapError)
{
    auto error_mapper = [](TestError err) { return static_cast<int>(err); };

    Expected<void, TestError> e_success;
    auto res1 = e_success.MapError(error_mapper);
    EXPECT_TRUE(res1.HasValue());

    Expected<void, TestError> e_error{ Unexpected(TestError::DefaultError) };
    auto res2 = e_error.MapError(error_mapper);
    EXPECT_TRUE(res2.HasError());
    EXPECT_EQ(res2.Error(), 0);
}

TEST_F(ExpectedAPI_Test, ExpectedVoidConstexpr)
{
    // constexpr 성공 상태
    constexpr Expected<void, TestError> e_success;
    static_assert(e_success.HasValue());
    static_assert(!e_success.HasError());

    // constexpr 에러 상태
    constexpr Expected<void, TestError> e_error{ Unexpected(TestError::DefaultError) };
    static_assert(!e_error.HasValue());
    static_assert(e_error.HasError());
    static_assert(e_error.Error() == TestError::DefaultError);
}

// ============================================================================
// Unexpected Tests
// ============================================================================

TEST_F(ExpectedAPI_Test, UnexpectedConstexpr)
{
    constexpr Unexpected<TestError> u(TestError::DefaultError);
    static_assert(u.Error() == TestError::DefaultError);

    constexpr auto moved_error = []() constexpr {
        Unexpected<TestError> u(TestError::AnotherError);
        return std::move(u).Error();
    }();
    static_assert(moved_error == TestError::AnotherError);
}

TEST_F(ExpectedAPI_Test, UnexpectedDeducingThis)
{
    // lvalue
    {
        Unexpected<std::string> u("error");
        std::string& err = u.Error();
        err = "modified";
        EXPECT_EQ(u.Error(), "modified");
    }

    // const lvalue
    {
        const Unexpected<std::string> u("error");
        const std::string& err = u.Error();
        EXPECT_EQ(err, "error");
    }

    // rvalue
    {
        Unexpected<std::string> u("error");
        std::string err = std::move(u).Error();
        EXPECT_EQ(err, "error");
    }
}
