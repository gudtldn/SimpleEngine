#include "doctest.h"

import SimpleEngine.Core.Function;
import std;


template <typename R, typename... P>
std::ostream& operator<<(std::ostream& os, const Function<R(P...)>& f)
{
    os << "Function object " << (f ? "(valid)" : "(empty)");
    return os;
}


namespace
{
constexpr size_t SBO_BUFFER_SIZE = sizeof(void*) * 3;

int free_function(int i)
{
    return i * 2;
}

struct Functor
{
    int operator()(int i) const
    {
        return i * 3;
    }
};

// A large functor that will force heap allocation
struct LargeFunctor
{
    char data[SBO_BUFFER_SIZE + 1];

    LargeFunctor()
    {
        // To avoid unused variable warnings
        for (size_t i = 0; i < sizeof(data); ++i)
        {
            data[i] = static_cast<char>(i);
        }
    }

    int operator()(int i) const
    {
        return i * 4;
    }
};
}


TEST_SUITE("SimpleEngine.Core.Function")
{
TEST_CASE("Default and Nullptr Construction")
{
    const Function<int(int)> f1;
    CHECK_FALSE(f1);

    const Function<void()> f2(nullptr);
    CHECK_FALSE(f2);
}

TEST_CASE("Invocation")
{
    SUBCASE("Free function")
    {
        Function<int(int)> f(free_function);
        CHECK(f);
        CHECK(f(10) == 20);
    }

    SUBCASE("Lambda (SBO)")
    {
        int captured_value = 5;
        Function<int(int)> f([captured_value](int i) { return i + captured_value; });
        CHECK(f);
        CHECK(f(10) == 15);
    }

    SUBCASE("Functor (SBO)")
    {
        Functor functor;
        Function<int(int)> f(functor);
        CHECK(f);
        CHECK(f(10) == 30);
    }

    SUBCASE("Large Lambda (Heap allocated)")
    {
        char data[SBO_BUFFER_SIZE + 1]{}; // Force heap allocation
        Function<int(int)> f([data](int i) { return i + data[0]; });
        CHECK(f);
        CHECK(f(10) == 10);
    }

    SUBCASE("Large Functor (Heap allocated)")
    {
        LargeFunctor large_functor;
        Function<int(int)> f(large_functor);
        CHECK(f);
        CHECK(f(10) == 40);
    }
}

TEST_CASE("Empty Function Call")
{
    const Function<int()> f;
    CHECK_THROWS_AS(f(), std::bad_function_call);
}

TEST_CASE("Copy Semantics")
{
    SUBCASE("Copy constructing from SBO")
    {
        int val = 10;
        Function<int()> f1 = [val] { return val; };
        Function<int()> f2 = f1;
        CHECK(f1);
        CHECK(f2);
        CHECK(f1() == 10);
        CHECK(f2() == 10);
    }

    SUBCASE("Copy constructing from Heap")
    {
        LargeFunctor lf;
        Function<int(int)> f1 = lf;
        Function<int(int)> f2 = f1;
        CHECK(f1);
        CHECK(f2);
        CHECK(f1(5) == 20);
        CHECK(f2(5) == 20);
    }

    SUBCASE("Copy assigning from SBO to empty")
    {
        Function<int()> f1;
        Function<int()> f2 = [] { return 20; };
        f1 = f2;
        CHECK(f1);
        CHECK(f2);
        CHECK(f1() == 20);
    }

    SUBCASE("Copy assigning from Heap to SBO")
    {
        Function<int(int)> f1 = [](int i) { return i; };
        Function<int(int)> f2 = LargeFunctor{};
        f1 = f2;
        CHECK(f1);
        CHECK(f2);
        CHECK(f1(10) == 40);
    }
}

TEST_CASE("Move Semantics")
{
    SUBCASE("Move constructing from SBO")
    {
        Function<std::string()> f1 = []() -> std::string { return "hello"; };
        Function<std::string()> f2 = std::move(f1);

        CHECK_FALSE(f1);
        CHECK(f2);
        CHECK(f2() == std::string("hello"));
        CHECK_THROWS_AS(f1(), std::bad_function_call);
    }

    SUBCASE("Move constructing from Heap")
    {
        Function<int(int)> f1 = LargeFunctor{};
        Function<int(int)> f2 = std::move(f1);

        CHECK_FALSE(f1);
        CHECK(f2);
        CHECK(f2(10) == 40);
        CHECK_THROWS_AS(f1(10), std::bad_function_call);
    }

    SUBCASE("Move assigning from SBO to Heap")
    {
        Function<int(int)> f1 = LargeFunctor{};
        Function<int(int)> f2 = [](int i) { return i * 5; };

        f1 = std::move(f2);
        CHECK(f1);
        CHECK_FALSE(f2);
        CHECK(f1(10) == 50);
        CHECK_THROWS_AS(f2(10), std::bad_function_call);
    }
}

TEST_CASE("Reset and Reassignment")
{
    Function<int(int)> f(free_function);
    CHECK(f);
    f = nullptr;
    CHECK_FALSE(f);

    f = [](int i) { return i + 1; };
    CHECK(f);
    CHECK(f(1) == 2);
}
}
