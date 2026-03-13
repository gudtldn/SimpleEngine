#include "gtest/gtest.h"

#include <memory>
#include <string>
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Functional/UniqueFunction.h"
#include "SimpleEngine/Core/Functional/FunctionRef.h"

using namespace se;

class FunctionAPI_Test       : public ::testing::Test {};
class UniqueFunctionAPI_Test : public ::testing::Test {};
class FunctionRefAPI_Test    : public ::testing::Test {};


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

// TEST_F(FunctionAPI_Test, EmptyFunctionCall)
// {
//     const Function<int()> f{};
//     EXPECT_THROW(f(), std::bad_function_call);
// }

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
        // EXPECT_THROW(f1(), std::bad_function_call);
    }

    // Move constructing from Heap
    {
        Function<int(int)> f1 = LargeFunctor{};
        Function<int(int)> f2 = std::move(f1);

        EXPECT_FALSE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f2(10), 40);
        // EXPECT_THROW(f1(10), std::bad_function_call);
    }

    // Move assigning from SBO to Heap
    {
        Function<int(int)> f1 = LargeFunctor{};
        Function<int(int)> f2 = [](int i) { return i * 5; };

        f1 = std::move(f2);
        EXPECT_TRUE(f1);
        EXPECT_FALSE(f2);
        EXPECT_EQ(f1(10), 50);
        // EXPECT_THROW(f2(10), std::bad_function_call);
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

// =============================================================================
// UniqueFunction
// =============================================================================

TEST_F(UniqueFunctionAPI_Test, DefaultAndNullptrConstruction)
{
    const UniqueFunction<int(int)> f1{};
    EXPECT_FALSE(f1);

    const UniqueFunction<void()> f2(nullptr);
    EXPECT_FALSE(f2);
}

TEST_F(UniqueFunctionAPI_Test, Invocation)
{
    // Free function (SBO)
    {
        UniqueFunction<int(int)> f(free_function);
        EXPECT_TRUE(f);
        EXPECT_EQ(f(10), 20);
    }

    // Lambda (SBO)
    {
        int val = 7;
        UniqueFunction<int(int)> f([val](int i) { return i + val; });
        EXPECT_TRUE(f);
        EXPECT_EQ(f(3), 10);
    }

    // Large Lambda (Heap allocated)
    {
        char data[detail::SBO_BUFFER_SIZE + 1]{};
        UniqueFunction<int(int)> f([data](int i) { return i + data[0]; });
        EXPECT_TRUE(f);
        EXPECT_EQ(f(5), 5);
    }

    // Large Functor (Heap allocated)
    {
        UniqueFunction<int(int)> f(LargeFunctor{});
        EXPECT_TRUE(f);
        EXPECT_EQ(f(10), 40);
    }
}

TEST_F(UniqueFunctionAPI_Test, UniquePtrCapture)
{
    // std::unique_ptr 캡처 — Function<> 으로는 저장 불가, UniqueFunction<> 만 가능
    auto ptr = std::make_unique<int>(42);
    UniqueFunction<int()> f([p = std::move(ptr)]() { return *p; });
    EXPECT_TRUE(f);
    EXPECT_EQ(f(), 42);
}

TEST_F(UniqueFunctionAPI_Test, MoveSemantics)
{
    // Move constructing from SBO
    {
        UniqueFunction<std::string()> f1 = []() -> std::string { return "hello"; };
        UniqueFunction<std::string()> f2 = std::move(f1);

        EXPECT_FALSE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f2(), std::string("hello"));
    }

    // Move constructing from Heap
    {
        UniqueFunction<int(int)> f1 = LargeFunctor{};
        UniqueFunction<int(int)> f2 = std::move(f1);

        EXPECT_FALSE(f1);
        EXPECT_TRUE(f2);
        EXPECT_EQ(f2(10), 40);
    }

    // Move assigning — Heap -> SBO
    {
        UniqueFunction<int(int)> f1 = LargeFunctor{};
        UniqueFunction<int(int)> f2 = [](int i) { return i * 5; };

        f1 = std::move(f2);
        EXPECT_TRUE(f1);
        EXPECT_FALSE(f2);
        EXPECT_EQ(f1(10), 50);
    }
}

TEST_F(UniqueFunctionAPI_Test, NoCopySemantics)
{
    EXPECT_FALSE(std::is_copy_constructible_v<UniqueFunction<void()>>);
    EXPECT_FALSE(std::is_copy_assignable_v<UniqueFunction<void()>>);
}

TEST_F(UniqueFunctionAPI_Test, ResetAndReassignment)
{
    UniqueFunction<int(int)> f(free_function);
    EXPECT_TRUE(f);
    f = nullptr;
    EXPECT_FALSE(f);

    f = [](int i) { return i * 10; };
    EXPECT_TRUE(f);
    EXPECT_EQ(f(3), 30);
}

// =============================================================================
// FunctionRef
// =============================================================================

TEST_F(FunctionRefAPI_Test, InvocationFromLambda)
{
    int val = 4;
    auto lambda = [val](int i) { return i * val; };
    FunctionRef<int(int)> ref(lambda);
    EXPECT_TRUE(ref);
    EXPECT_EQ(ref(5), 20);
}

TEST_F(FunctionRefAPI_Test, InvocationFromFreeFunction)
{
    FunctionRef<int(int)> ref(free_function);
    EXPECT_TRUE(ref);
    EXPECT_EQ(ref(7), 14);
}

TEST_F(FunctionRefAPI_Test, InvocationFromFunctor)
{
    Functor functor;
    FunctionRef<int(int)> ref(functor);
    EXPECT_TRUE(ref);
    EXPECT_EQ(ref(4), 12);
}

TEST_F(FunctionRefAPI_Test, NonOwning_ReflectsOriginalMutation)
{
    // FunctionRef 는 원본을 참조하므로, 원본 변경이 호출 결과에 반영됨
    int multiplier = 2;
    auto lambda = [&multiplier](int i) { return i * multiplier; };

    FunctionRef<int(int)> ref(lambda);
    EXPECT_EQ(ref(5), 10);

    multiplier = 3;
    EXPECT_EQ(ref(5), 15);
}

TEST_F(FunctionRefAPI_Test, PassAsParameter)
{
    // 함수 파라미터로 전달하는 일반적인 사용 패턴
    auto apply = [](FunctionRef<int(int)> fn, int x) { return fn(x); };

    EXPECT_EQ(apply([](int i) { return i + 1; }, 9), 10);
    EXPECT_EQ(apply(free_function, 5), 10);
}

TEST_F(FunctionRefAPI_Test, BoolConversion)
{
    int x = 0;
    auto lambda = [&x] { ++x; };
    FunctionRef<void()> ref(lambda);
    EXPECT_TRUE(static_cast<bool>(ref));
    EXPECT_TRUE(ref.IsValid());
    ref();
    EXPECT_EQ(x, 1);
}
