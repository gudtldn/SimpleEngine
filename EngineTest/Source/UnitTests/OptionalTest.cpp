#include "gtest/gtest.h"

#include <string>
#include "SimpleEngine/Core/Container/Optional.h"

class OptionalAPI_Test : public ::testing::Test {};


TEST_F(OptionalAPI_Test, OptionalForValueTypes)
{
    // Default and Nullopt construction
    {
        Optional<int> opt1;
        EXPECT_FALSE(opt1.HasValue());
        EXPECT_FALSE(opt1);
        EXPECT_EQ(opt1, std::nullopt);

        Optional<std::string> opt2(std::nullopt);
        EXPECT_FALSE(opt2.HasValue());
        EXPECT_EQ(opt2, std::nullopt);
    }

    // Value construction
    {
        Optional<int> opt1(42);
        EXPECT_TRUE(opt1.HasValue());
        EXPECT_EQ(opt1.Value(), 42);
        EXPECT_EQ(*opt1, 42);

        std::string s = "hello";
        Optional<std::string> opt2(s);
        EXPECT_TRUE(opt2.HasValue());
        EXPECT_EQ(*opt2, "hello");

        Optional<std::string> opt3("world");
        EXPECT_TRUE(opt3.HasValue());
        EXPECT_EQ(*opt3, "world");
    }

    // In-place construction
    {
        Optional<std::pair<int, std::string>> opt(std::in_place, 1, "test");
        EXPECT_TRUE(opt.HasValue());
        EXPECT_EQ(opt->first, 1);
        EXPECT_EQ(opt->second, "test");
    }

    // Copy and Move construction
    {
        Optional<int> opt1(10);
        Optional<int> opt2 = opt1; // Copy
        EXPECT_TRUE(opt1.HasValue());
        EXPECT_TRUE(opt2.HasValue());
        EXPECT_EQ(*opt1, 10);
        EXPECT_EQ(*opt2, 10);

        Optional<int> opt3 = std::move(opt1); // Move
        EXPECT_FALSE(opt1.HasValue());         // Original is now empty
        EXPECT_TRUE(opt3.HasValue());
        EXPECT_EQ(*opt3, 10);

        Optional<int> empty_opt;
        Optional<int> empty_opt_copy = empty_opt;
        EXPECT_FALSE(empty_opt_copy.HasValue());
    }

    // Copy and Move assignment
    {
        Optional<std::string> opt1("one");
        Optional<std::string> opt2("two");
        opt1 = opt2; // Copy assignment
        EXPECT_EQ(*opt1, "two");
        EXPECT_EQ(*opt2, "two");

        opt1 = std::move(opt2); // Move assignment
        EXPECT_EQ(*opt1, "two");
        EXPECT_FALSE(opt2.HasValue());

        Optional<std::string> opt3;
        opt3 = opt1;
        EXPECT_TRUE(opt3.HasValue());
        EXPECT_EQ(*opt3, "two");

        opt1 = std::nullopt; // Reset via assignment
        EXPECT_FALSE(opt1.HasValue());
    }

    // Value access
    {
        Optional<int> opt(42);
        EXPECT_EQ(opt.Value(), 42);

        const Optional<int> c_opt(42);
        EXPECT_EQ(c_opt.Value(), 42);

        EXPECT_EQ(std::move(opt).Value(), 42);
    }

    // Pointer-like access
    {
        Optional<std::string> opt("test");
        EXPECT_EQ(opt->length(), 4);
        *opt = "changed";
        EXPECT_EQ(*opt, "changed");
    }

    // value_or
    {
        Optional<int> opt(42);
        EXPECT_EQ(opt.ValueOr(99), 42);

        Optional<int> empty_opt;
        EXPECT_EQ(empty_opt.ValueOr(99), 99);
        EXPECT_EQ(std::move(empty_opt).ValueOr(100), 100);
    }

    // reset and emplace
    {
        Optional<std::string> opt("initial");
        EXPECT_TRUE(opt.HasValue());

        opt.Reset();
        EXPECT_FALSE(opt.HasValue());

        opt.Emplace(5, 'c');
        EXPECT_TRUE(opt.HasValue());
        EXPECT_EQ(*opt, "ccccc");
    }

    // Comparison operators
    {
        Optional<int> o1(10), o2(10), o3(20), o4;

        EXPECT_EQ(o1, o2);
        EXPECT_NE(o1, o3);
        EXPECT_NE(o1, o4);

        EXPECT_EQ(o4, std::nullopt);
        EXPECT_NE(o1, std::nullopt);
        EXPECT_NE(o1, std::nullopt);

        EXPECT_EQ(o1, 10);
        EXPECT_EQ(10, o1);
        EXPECT_NE(o1, 20);
        EXPECT_NE(20, o1);
    }

    // transform
    {
        Optional<int> opt(21);
        auto transformed = opt.Transform([](int n) { return std::to_string(n * 2); });
        EXPECT_TRUE(transformed.HasValue());
        EXPECT_EQ(*transformed, "42");

        Optional<int> empty_opt;
        auto transformed_empty = empty_opt.Transform([](int n) { return std::to_string(n); });
        EXPECT_FALSE(transformed_empty.HasValue());
    }

    // and_then
    {
        auto half = [](int n) -> Optional<double>
        {
            if (n % 2 == 0)
            {
                return Optional<double>(n / 2.0);
            }
            return std::nullopt;
        };

        Optional<int> opt1(10);
        auto res1 = opt1.AndThen(half);
        EXPECT_TRUE(res1.HasValue());
        EXPECT_EQ(*res1, 5.0);

        Optional<int> opt2(9);
        auto res2 = opt2.AndThen(half);
        EXPECT_FALSE(res2.HasValue());
    }

    // or_else
    {
        Optional<int> opt1(10);
        auto res1 = opt1.OrElse([] { return Optional<int>(99); });
        EXPECT_TRUE(res1.HasValue());
        EXPECT_EQ(*res1, 10);

        Optional<int> opt2;
        auto res2 = opt2.OrElse([] { return Optional<int>(99); });
        EXPECT_TRUE(res2.HasValue());
        EXPECT_EQ(*res2, 99);
    }
}

TEST_F(OptionalAPI_Test, OptionalForReferenceTypes) // Optional<T&>
{
    // Construction and State
    {
        int x = 10;
        Optional<int&> opt(x);
        EXPECT_TRUE(opt.HasValue());
        EXPECT_TRUE(opt);

        Optional<int&> empty_opt;
        EXPECT_FALSE(empty_opt.HasValue());
        EXPECT_FALSE(empty_opt);

        Optional<int&> null_opt(std::nullopt);
        EXPECT_FALSE(null_opt.HasValue());
    }

    // Value access and modification
    {
        int x = 10;
        Optional<int&> opt(x);

        EXPECT_EQ(opt.Value(), 10);
        EXPECT_EQ(&(*opt), &x);

        *opt = 20;
        EXPECT_EQ(x, 20);

        x = 30;
        EXPECT_EQ(*opt, 30);
    }

    // Assignment
    {
        int x = 10;
        int y = 20;
        Optional<int&> opt_x(x);
        Optional<int&> opt_y(y);

        opt_x = opt_y; // Assigns the reference
        EXPECT_TRUE(opt_x.HasValue());
        EXPECT_EQ(&(*opt_x), &y);

        opt_x = std::nullopt;
        EXPECT_FALSE(opt_x.HasValue());
    }

    // Optional<T> -> Optional<T&>
    {
        Optional<int> opt(10);
        Optional<int&> opt_ref = opt;
        EXPECT_TRUE(opt_ref.HasValue());
        EXPECT_EQ(&(*opt_ref), &opt.Value());

        Optional<int> empty_opt;
        Optional<int&> empty_opt_ref = empty_opt;
        EXPECT_FALSE(empty_opt_ref.HasValue());
    }

    // value_or
    {
        int x = 10;
        int default_val = 99;
        Optional<int&> opt(x);
        EXPECT_EQ(opt.ValueOr(default_val), 10);
        EXPECT_EQ(&opt.ValueOr(default_val), &x);

        Optional<int&> empty_opt;
        EXPECT_EQ(empty_opt.ValueOr(default_val), 99);
        EXPECT_EQ(&empty_opt.ValueOr(default_val), &default_val);
    }

    // Comparison
    {
        int x = 10, y = 10, z = 20;
        Optional<int&> o1(x), o2(y), o3(z), o4;

        EXPECT_EQ(o1, o2);
        EXPECT_NE(o1, o3);
        EXPECT_NE(o1, o4);

        EXPECT_EQ(o1, 10);
        EXPECT_EQ(10, o1);
        EXPECT_NE(o1, 20);
    }

    // transform
    {
        int x = 21;
        Optional<int&> opt(x);
        auto transformed = opt.Transform([](int n) { return std::to_string(n * 2); });
        EXPECT_TRUE(transformed.HasValue());
        EXPECT_EQ(*transformed, "42");

        Optional<int&> empty_opt;
        auto transformed_empty = empty_opt.Transform([](int n) { return std::to_string(n); });
        EXPECT_FALSE(transformed_empty.HasValue());
    }
}

// ============================================================================
// Constexpr Tests
// ============================================================================

TEST_F(OptionalAPI_Test, ConstexprConstruction)
{
    // constexpr 기본 생성
    constexpr Optional<int> empty_opt;
    static_assert(!empty_opt.HasValue());

    // constexpr nullopt 생성
    constexpr Optional<int> null_opt(std::nullopt);
    static_assert(!null_opt.HasValue());

    // constexpr 값 생성
    constexpr Optional<int> opt(42);
    static_assert(opt.HasValue());
    static_assert(opt.Value() == 42);

    // constexpr in_place 생성
    constexpr Optional<std::pair<int, int>> pair_opt(std::in_place, 1, 2);
    static_assert(pair_opt.HasValue());
    static_assert(pair_opt.Value().first == 1);
    static_assert(pair_opt.Value().second == 2);
}

TEST_F(OptionalAPI_Test, ConstexprOperations)
{
    // constexpr ValueOr
    constexpr Optional<int> opt(42);
    static_assert(opt.ValueOr(99) == 42);

    constexpr Optional<int> empty_opt;
    static_assert(empty_opt.ValueOr(99) == 99);

    // constexpr ValueOrDefault
    static_assert(empty_opt.ValueOrDefault() == 0);
    static_assert(opt.ValueOrDefault() == 42);

    // constexpr 비교
    static_assert(opt == 42);
    static_assert(empty_opt == std::nullopt);
    static_assert(opt != std::nullopt);
}

TEST_F(OptionalAPI_Test, ConstexprTransformAndAndThen)
{
    // constexpr Transform
    constexpr auto transform_result = []() constexpr {
        Optional<int> opt(21);
        return opt.Transform([](int n) { return n * 2; });
    }();
    static_assert(transform_result.HasValue());
    static_assert(transform_result.Value() == 42);

    // constexpr AndThen
    constexpr auto and_then_result = []() constexpr {
        Optional<int> opt(10);
        return opt.AndThen([](int n) -> Optional<int> {
            if (n > 0) return n * 2;
            return std::nullopt;
        });
    }();
    static_assert(and_then_result.HasValue());
    static_assert(and_then_result.Value() == 20);

    // constexpr OrElse
    constexpr auto or_else_result = []() constexpr {
        Optional<int> opt;
        return opt.OrElse([]() { return Optional<int>(99); });
    }();
    static_assert(or_else_result.HasValue());
    static_assert(or_else_result.Value() == 99);
}

TEST_F(OptionalAPI_Test, ValueOrDefault)
{
    // Runtime tests for ValueOrDefault
    Optional<int> opt(42);
    EXPECT_EQ(opt.ValueOrDefault(), 42);

    Optional<int> empty_opt;
    EXPECT_EQ(empty_opt.ValueOrDefault(), 0);

    Optional<std::string> str_opt("hello");
    EXPECT_EQ(str_opt.ValueOrDefault(), "hello");

    Optional<std::string> empty_str_opt;
    EXPECT_EQ(empty_str_opt.ValueOrDefault(), "");
}

TEST_F(OptionalAPI_Test, DeducingThisValueCategories)
{
    // lvalue
    {
        Optional<std::string> opt("test");
        std::string& val = opt.Value();
        val = "modified";
        EXPECT_EQ(opt.Value(), "modified");
    }

    // const lvalue
    {
        const Optional<std::string> opt("test");
        const std::string& val = opt.Value();
        EXPECT_EQ(val, "test");
    }

    // rvalue
    {
        Optional<std::string> opt("test");
        std::string val = std::move(opt).Value();
        EXPECT_EQ(val, "test");
    }

    // Transform with different value categories
    {
        Optional<int> opt(10);

        // lvalue
        auto res1 = opt.Transform([](int& n) { return n * 2; });
        EXPECT_EQ(res1.Value(), 20);

        // rvalue
        auto res2 = std::move(opt).Transform([](int&& n) { return n * 3; });
        EXPECT_EQ(res2.Value(), 30);
    }
}

TEST_F(OptionalAPI_Test, OptionalRefCopy)
{
    int x = 42;
    Optional<int&> ref_opt(x);

    // Copy를 통해 값 복사
    Optional<int> copied = ref_opt.Copy();
    EXPECT_TRUE(copied.HasValue());
    EXPECT_EQ(copied.Value(), 42);

    // 원본 변경해도 복사본은 영향 없음
    x = 100;
    EXPECT_EQ(ref_opt.Value(), 100);
    EXPECT_EQ(copied.Value(), 42);

    // 빈 Optional<T&>의 Copy
    Optional<int&> empty_ref;
    Optional<int> empty_copied = empty_ref.Copy();
    EXPECT_FALSE(empty_copied.HasValue());
}
