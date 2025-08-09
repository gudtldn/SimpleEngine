#include "doctest.h"

import SimpleEngine.Prelude;
import std;


TEST_SUITE("SimpleEngine.Types.Optional")
{
TEST_CASE("Optional for value types")
{
    SUBCASE("Default and Nullopt construction")
    {
        Optional<int> opt1;
        CHECK_FALSE(opt1.HasValue());
        CHECK_FALSE(opt1);
        CHECK(opt1 == std::nullopt);

        Optional<std::string> opt2(std::nullopt);
        CHECK_FALSE(opt2.HasValue());
        CHECK(opt2 == std::nullopt);
    }

    SUBCASE("Value construction")
    {
        Optional<int> opt1(42);
        CHECK(opt1.HasValue());
        CHECK(opt1.Value() == 42);
        CHECK(*opt1 == 42);

        std::string s = "hello";
        Optional<std::string> opt2(s);
        CHECK(opt2.HasValue());
        CHECK(*opt2 == "hello");

        Optional<std::string> opt3("world");
        CHECK(opt3.HasValue());
        CHECK(*opt3 == "world");
    }

    SUBCASE("In-place construction")
    {
        Optional<std::pair<int, std::string>> opt(std::in_place, 1, "test");
        CHECK(opt.HasValue());
        CHECK(opt->first == 1);
        CHECK(opt->second == "test");
    }

    SUBCASE("Copy and Move construction")
    {
        Optional<int> opt1(10);
        Optional<int> opt2 = opt1; // Copy
        CHECK(opt1.HasValue());
        CHECK(opt2.HasValue());
        CHECK(*opt1 == 10);
        CHECK(*opt2 == 10);

        Optional<int> opt3 = std::move(opt1); // Move
        CHECK_FALSE(opt1.HasValue());         // Original is now empty
        CHECK(opt3.HasValue());
        CHECK(*opt3 == 10);

        Optional<int> empty_opt;
        Optional<int> empty_opt_copy = empty_opt;
        CHECK_FALSE(empty_opt_copy.HasValue());
    }

    SUBCASE("Copy and Move assignment")
    {
        Optional<std::string> opt1("one");
        Optional<std::string> opt2("two");
        opt1 = opt2; // Copy assignment
        CHECK(*opt1 == "two");
        CHECK(*opt2 == "two");

        opt1 = std::move(opt2); // Move assignment
        CHECK(*opt1 == "two");
        CHECK_FALSE(opt2.HasValue());

        Optional<std::string> opt3;
        opt3 = opt1;
        CHECK(opt3.HasValue());
        CHECK(*opt3 == "two");

        opt1 = std::nullopt; // Reset via assignment
        CHECK_FALSE(opt1.HasValue());
    }

    SUBCASE("Value access")
    {
        Optional<int> opt(42);
        CHECK(opt.Value() == 42);

        const Optional<int> c_opt(42);
        CHECK(c_opt.Value() == 42);

        CHECK(std::move(opt).Value() == 42);

        Optional<int> empty_opt;
        CHECK_THROWS_AS((void)empty_opt.Value(), std::bad_optional_access);
    }

    SUBCASE("Pointer-like access")
    {
        Optional<std::string> opt("test");
        CHECK(opt->length() == 4);
        *opt = "changed";
        CHECK(*opt == "changed");
    }

    SUBCASE("value_or")
    {
        Optional<int> opt(42);
        CHECK(opt.ValueOr(99) == 42);

        Optional<int> empty_opt;
        CHECK(empty_opt.ValueOr(99) == 99);
        CHECK(std::move(empty_opt).ValueOr(100) == 100);
    }

    SUBCASE("reset and emplace")
    {
        Optional<std::string> opt("initial");
        CHECK(opt.HasValue());

        opt.Reset();
        CHECK_FALSE(opt.HasValue());

        opt.Emplace(5, 'c');
        CHECK(opt.HasValue());
        CHECK(*opt == "ccccc");
    }

    SUBCASE("Comparison operators")
    {
        Optional<int> o1(10), o2(10), o3(20), o4;

        CHECK(o1 == o2);
        CHECK_FALSE(o1 == o3);
        CHECK_FALSE(o1 == o4);
        CHECK(o1 != o3);
        CHECK(o1 != o4);

        CHECK(o4 == std::nullopt);
        CHECK_FALSE(o1 == std::nullopt);
        CHECK(o1 != std::nullopt);

        CHECK(o1 == 10);
        CHECK(10 == o1);
        CHECK_FALSE(o1 == 20);
        CHECK_FALSE(20 == o1);
    }

    SUBCASE("transform")
    {
        Optional<int> opt(21);
        auto transformed = opt.Transform([](int n) { return std::to_string(n * 2); });
        CHECK(transformed.HasValue());
        CHECK(*transformed == "42");

        Optional<int> empty_opt;
        auto transformed_empty = empty_opt.Transform([](int n) { return std::to_string(n); });
        CHECK_FALSE(transformed_empty.HasValue());
    }

    SUBCASE("and_then")
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
        CHECK(res1.HasValue());
        CHECK(*res1 == 5.0);

        Optional<int> opt2(9);
        auto res2 = opt2.AndThen(half);
        CHECK_FALSE(res2.HasValue());
    }

    SUBCASE("or_else")
    {
        Optional<int> opt1(10);
        auto res1 = opt1.OrElse([] { return Optional<int>(99); });
        CHECK(res1.HasValue());
        CHECK(*res1 == 10);

        Optional<int> opt2;
        auto res2 = opt2.OrElse([] { return Optional<int>(99); });
        CHECK(res2.HasValue());
        CHECK(*res2 == 99);
    }
}

TEST_CASE("Optional for reference types (Optional<T&>)")
{
    SUBCASE("Construction and State")
    {
        int x = 10;
        Optional<int&> opt(x);
        CHECK(opt.HasValue());
        CHECK(opt);

        Optional<int&> empty_opt;
        CHECK_FALSE(empty_opt.HasValue());
        CHECK_FALSE(empty_opt);

        Optional<int&> null_opt(std::nullopt);
        CHECK_FALSE(null_opt.HasValue());
    }

    SUBCASE("Value access and modification")
    {
        int x = 10;
        Optional<int&> opt(x);

        CHECK(opt.Value() == 10);
        CHECK(&(*opt) == &x);

        *opt = 20;
        CHECK(x == 20);

        x = 30;
        CHECK(*opt == 30);

        Optional<int&> empty_opt;
        CHECK_THROWS_AS((void)empty_opt.Value(), std::bad_optional_access);
    }

    SUBCASE("Assignment")
    {
        int x = 10;
        int y = 20;
        Optional<int&> opt_x(x);
        Optional<int&> opt_y(y);

        opt_x = opt_y; // Assigns the reference
        CHECK(opt_x.HasValue());
        CHECK(&(*opt_x) == &y);

        opt_x = std::nullopt;
        CHECK_FALSE(opt_x.HasValue());
    }

    SUBCASE("value_or")
    {
        int x = 10;
        int default_val = 99;
        Optional<int&> opt(x);
        CHECK(opt.ValueOr(default_val) == 10);
        CHECK(&opt.ValueOr(default_val) == &x);

        Optional<int&> empty_opt;
        CHECK(empty_opt.ValueOr(default_val) == 99);
        CHECK(&empty_opt.ValueOr(default_val) == &default_val);
    }

    SUBCASE("Comparison")
    {
        int x = 10, y = 10, z = 20;
        Optional<int&> o1(x), o2(y), o3(z), o4;

        CHECK(o1 == o2);
        CHECK_FALSE(o1 == o3);
        CHECK_FALSE(o1 == o4);

        CHECK(o1 == 10);
        CHECK(10 == o1);
        CHECK_FALSE(o1 == 20);
    }

    SUBCASE("transform")
    {
        int x = 21;
        Optional<int&> opt(x);
        auto transformed = opt.Transform([](int n) { return std::to_string(n * 2); });
        CHECK(transformed.HasValue());
        CHECK(*transformed == "42");

        Optional<int&> empty_opt;
        auto transformed_empty = empty_opt.Transform([](int n) { return std::to_string(n); });
        CHECK_FALSE(transformed_empty.HasValue());
    }
}
}
