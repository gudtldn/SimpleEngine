#include "doctest/doctest.h"

#include <ostream>
#include <string_view>

#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Reflection/TypeSignature.h"
#include "SimpleEngine/Utility/Hash.h"


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

TEST_SUITE("SimpleEngine.Utility.TypeUtils")
{
using namespace se::refl;

TEST_CASE("Test CompileTime Type Name")
{
    SUBCASE("int")
    {
        //     PrintTestResult("int&", "int", GetTypeNameString<int&>(true), GetTypeNameString<int&>(false));
        //     PrintTestResult("const int&", "int", GetTypeNameString<const int&>(true), GetTypeNameString<const int&>(false));
        //     PrintTestResult("volatile int&", "int", GetTypeNameString<volatile int&>(true), GetTypeNameString<volatile int&>(false));
        //     PrintTestResult("int&&", "int", GetTypeNameString<int&&>(true), GetTypeNameString<int&&>(false));
        constexpr auto name = GetFullTypeName<int>();
        constexpr auto without_ns_name = GetTypeName<int>();
        CHECK(name == "int");
        CHECK(without_ns_name == "int");

        constexpr auto name1 = GetFullTypeName<const int[10]>();
        constexpr auto without_ns_name1 = GetTypeName<const int[10]>();
        CHECK(name1 == "int");
        CHECK(without_ns_name1 == "int");

        constexpr auto name2 = GetFullTypeName<const int*>();
        constexpr auto without_ns_name2 = GetTypeName<const int*>();
        CHECK(name2 == "int");
        CHECK(without_ns_name2 == "int");

        constexpr auto name3 = GetFullTypeName<const int* const>();
        constexpr auto without_ns_name3 = GetTypeName<const int* const>();
        CHECK(name3 == "int");
        CHECK(without_ns_name3 == "int");

        constexpr auto name4 = GetFullTypeName<const int* const&&>();
        constexpr auto without_ns_name4 = GetTypeName<const int* const&&>();
        CHECK(name4 == "int");
        CHECK(without_ns_name4 == "int");

        constexpr auto name5 = GetFullTypeName<const int&>();
        constexpr auto without_ns_name5 = GetTypeName<const int&>();
        CHECK(name5 == "int");
        CHECK(without_ns_name5 == "int");
    }

    SUBCASE("const volatile WeirdNamespace::MyEnum***** const volatile")
    {
        constexpr auto name = GetFullTypeName<const volatile WeirdNamespace::MyEnum***** const volatile>();
        constexpr auto without_ns_name = GetTypeName<const volatile WeirdNamespace::MyEnum***** const volatile>();
        CHECK(name == "WeirdNamespace::MyEnum");
        CHECK(without_ns_name == "MyEnum");
    }

    SUBCASE("const volatile int* const* volatile** const")
    {
        using TestType2 = const volatile int* const* volatile** const;
        constexpr auto name = GetFullTypeName<TestType2>();
        constexpr auto without_ns_name = GetTypeName<TestType2>();

        CHECK(name == "int");
        CHECK(without_ns_name == "int");
    }

    SUBCASE("const WeirdNamespace::MyClass* const&&")
    {
        using TestType3 = const WeirdNamespace::MyClass* const&&;
        constexpr auto name = GetFullTypeName<TestType3>();
        constexpr auto without_ns_name = GetTypeName<TestType3>();

        CHECK(name == "WeirdNamespace::MyClass");
        CHECK(without_ns_name == "MyClass");
    }

    SUBCASE("WeirdNamespace::MyClass* const (&)[5]")
    {
        using TestType4 = WeirdNamespace::MyClass* const (&)[5];
        constexpr auto name = GetFullTypeName<TestType4>();
        constexpr auto without_ns_name = GetTypeName<TestType4>();

        CHECK(name == "WeirdNamespace::MyClass");
        CHECK(without_ns_name == "MyClass");
    }

    SUBCASE("MyEnum ****** * ** ")
    {
        constexpr auto name = GetFullTypeName<TestEnum****** * **>();
        constexpr auto without_ns_name = GetTypeName<TestEnum****** * **>();

        CHECK(name == "TestEnum");
        CHECK(without_ns_name == "TestEnum");
    }
}

TEST_CASE("TypeId Test")
{
    constexpr TypeId id = TypeId::Get<int>();
    CHECK(id.GetName() == "int");
    CHECK(id.GetHash() == se::utility::FNV_Hash("int"));
}
}
