#include "doctest.h"

import SimpleEngine.Prelude;
import std;


namespace WeirdNamespace
{
enum class MyEnum { A, B };

struct MyClass
{
    void member_func(int) const volatile
    {
    }
};

template <typename T, int N, typename U>
struct MyTemplate
{
};
}

enum class TestEnum
{
};

TEST_SUITE("SimpleEngine.Core.Reflection")
{
using namespace se::core::reflection;

TEST_CASE("Test CompileTime Type Name")
{
    SUBCASE("int")
    {
        //     PrintTestResult("int&", "int", GetTypeNameString<int&>(true), GetTypeNameString<int&>(false));
        //     PrintTestResult("const int&", "int", GetTypeNameString<const int&>(true), GetTypeNameString<const int&>(false));
        //     PrintTestResult("volatile int&", "int", GetTypeNameString<volatile int&>(true), GetTypeNameString<volatile int&>(false));
        //     PrintTestResult("int&&", "int", GetTypeNameString<int&&>(true), GetTypeNameString<int&&>(false));
        constexpr auto name = GetTypeSignature<int>();
        constexpr auto without_ns_name = GetTypeSignature<int>(false);
        CHECK(name == "int");
        CHECK(without_ns_name == "int");

        constexpr auto name1 = GetTypeSignature<const int[10]>();
        constexpr auto without_ns_name1 = GetTypeSignature<const int[10]>(false);
        CHECK(name1 == "int");
        CHECK(without_ns_name1 == "int");

        constexpr auto name2 = GetTypeSignature<const int*>();
        constexpr auto without_ns_name2 = GetTypeSignature<const int*>(false);
        CHECK(name2 == "int");
        CHECK(without_ns_name2 == "int");

        constexpr auto name3 = GetTypeSignature<const int* const>();
        constexpr auto without_ns_name3 = GetTypeSignature<const int* const>(false);
        CHECK(name3 == "int");
        CHECK(without_ns_name3 == "int");

        constexpr auto name4 = GetTypeSignature<const int* const&&>();
        constexpr auto without_ns_name4 = GetTypeSignature<const int* const&&>(false);
        CHECK(name4 == "int");
        CHECK(without_ns_name4 == "int");

        constexpr auto name5 = GetTypeSignature<const int&>();
        constexpr auto without_ns_name5 = GetTypeSignature<const int&>(false);
        CHECK(name5 == "int");
        CHECK(without_ns_name5 == "int");
    }

    SUBCASE("const volatile WeirdNamespace::MyEnum***** const volatile")
    {
        constexpr auto name = GetTypeSignature<const volatile WeirdNamespace::MyEnum***** const volatile>();
        constexpr auto without_ns_name = GetTypeSignature<const volatile WeirdNamespace::MyEnum***** const volatile>(false);
        CHECK(name == "WeirdNamespace::MyEnum");
        CHECK(without_ns_name == "MyEnum");
    }

    SUBCASE("const volatile int* const* volatile** const")
    {
        using TestType2 = const volatile int* const* volatile** const;
        constexpr auto name = GetTypeSignature<TestType2>();
        constexpr auto without_ns_name = GetTypeSignature<TestType2>(false);

        CHECK(name == "int");
        CHECK(without_ns_name == "int");
    }

    SUBCASE("const WeirdNamespace::MyClass* const&&")
    {
        using TestType3 = const WeirdNamespace::MyClass* const&&;
        constexpr auto name = GetTypeSignature<TestType3>();
        constexpr auto without_ns_name = GetTypeSignature<TestType3>(false);

        CHECK(name == "WeirdNamespace::MyClass");
        CHECK(without_ns_name == "MyClass");
    }

    SUBCASE("WeirdNamespace::MyClass* const (&)[5]")
    {
        using TestType4 = WeirdNamespace::MyClass* const (&)[5];
        constexpr auto name = GetTypeSignature<TestType4>();
        constexpr auto without_ns_name = GetTypeSignature<TestType4>(false);

        CHECK(name == "WeirdNamespace::MyClass");
        CHECK(without_ns_name == "MyClass");
    }

    SUBCASE("MyEnum ****** * ** ")
    {
        constexpr auto name = GetTypeSignature<TestEnum****** * **>();
        constexpr auto without_ns_name = GetTypeSignature<TestEnum****** * **>(false);

        CHECK(name == "TestEnum");
        CHECK(without_ns_name == "TestEnum");
    }
}

TEST_CASE("TypeId Test")
{
    constexpr TypeId id = TypeId::Get<int>();
    CHECK(id.GetName() == "int");
    CHECK(id.GetHash() == se::core::hash::FowlerNollVoHash("int"));
}
}
