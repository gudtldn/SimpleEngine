#include "gtest/gtest.h"

#include <string>
#include "SimpleEngine/Core/Functional/Function.h"

using namespace se;

class FunctionAPI_Test : public ::testing::Test {};


namespace
{
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
    char data[detail::SBO_BUFFER_SIZE + 1];

    LargeFunctor()
    {
        // To avoid unused variable warnings
        for (usize i = 0; i < sizeof(data); ++i)
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


template <typename R, typename... P>
std::ostream& operator<<(std::ostream& os, const Function<R(P...)>& f)
{
    os << "Function object " << (f ? "(valid)" : "(empty)");
    return os;
}

TEST_F(FunctionAPI_Test, DefaultAndNullptrConstruction)
{
    const Function<int(int)> f1{};
    EXPECT_FALSE(f1);

    const Function<void()> f2(nullptr);
    EXPECT_FALSE(f2);
}

TEST_F(FunctionAPI_Test, Invocation)
{
    // Free function
    {
        Function<int(int)> f(free_function);
        EXPECT_TRUE(f);
        EXPECT_EQ(f(10), 20);
    }

    // Lambda (SBO)
    {
        int captured_value = 5;
        Function<int(int)> f([captured_value](int i) { return i + captured_value; });
        EXPECT_TRUE(f);
        EXPECT_EQ(f(10), 15);
    }

    // Functor (SBO)
    {
        Functor functor;
        Function<int(int)> f(functor);
        EXPECT_TRUE(f);
        EXPECT_EQ(f(10), 30);
    }

    // Large Lambda (Heap allocated)
    {
        char data[detail::SBO_BUFFER_SIZE + 1]{}; // Force heap allocation
        Function<int(int)> f([data](int i) { return i + data[0]; });
        EXPECT_TRUE(f);
        EXPECT_EQ(f(10), 10);
    }

    // Large Functor (Heap allocated)
    {
        LargeFunctor large_functor;
        Function<int(int)> f(large_functor);
        EXPECT_TRUE(f);
        EXPECT_EQ(f(10), 40);
    }
}

TEST_F(FunctionAPI_Test, EmptyFunctionCall)
{
    const Function<int()> f{};
    EXPECT_THROW(f(), std::bad_function_call);
}

TEST_F(FunctionAPI_Test, CopySemantics)
{
    // Copy constructing from SBO
    {
        int val = 10;
        Function<int()> f1 = [val] { return val; };
        Function<int()> f2 = f1;
        EXPECT_TRUE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f1(), 10);
        EXPECT_EQ(f2(), 10);
    }

    // Copy constructing from Heap
    {
        LargeFunctor lf;
        Function<int(int)> f1 = lf;
        Function<int(int)> f2 = f1;
        EXPECT_TRUE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f1(5), 20);
        EXPECT_EQ(f2(5), 20);
    }

    // Copy assigning from SBO to empty
    {
        Function<int()> f1;
        Function<int()> f2 = [] { return 20; };
        f1 = f2;
        EXPECT_TRUE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f1(), 20);
    }

    // Copy assigning from Heap to SBO
    {
        Function<int(int)> f1 = [](int i) { return i; };
        Function<int(int)> f2 = LargeFunctor{};
        f1 = f2;
        EXPECT_TRUE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f1(10), 40);
    }
}

TEST_F(FunctionAPI_Test, MoveSemantics)
{
    // Move constructing from SBO
    {
        Function<std::string()> f1 = []() -> std::string { return "hello"; };
        Function<std::string()> f2 = std::move(f1);

        EXPECT_FALSE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f2(), std::string("hello"));
        EXPECT_THROW(f1(), std::bad_function_call);
    }

    // Move constructing from Heap
    {
        Function<int(int)> f1 = LargeFunctor{};
        Function<int(int)> f2 = std::move(f1);

        EXPECT_FALSE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f2(10), 40);
        EXPECT_THROW(f1(10), std::bad_function_call);
    }

    // Move assigning from SBO to Heap
    {
        Function<int(int)> f1 = LargeFunctor{};
        Function<int(int)> f2 = [](int i) { return i * 5; };

        f1 = std::move(f2);
        EXPECT_TRUE(f1);
        EXPECT_FALSE(f2);
        EXPECT_EQ(f1(10), 50);
        EXPECT_THROW(f2(10), std::bad_function_call);
    }
}

TEST_F(FunctionAPI_Test, ResetAndReassignment)
{
    Function<int(int)> f(free_function);
    EXPECT_TRUE(f);
    f = nullptr;
    EXPECT_FALSE(f);

    f = [](int i) { return i + 1; };
    EXPECT_TRUE(f);
    EXPECT_EQ(f(1), 2);
}
