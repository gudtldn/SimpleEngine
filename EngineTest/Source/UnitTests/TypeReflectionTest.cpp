#include "gtest/gtest.h"

#include <ostream>
#include <string_view>

#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeSignature.h"
#include "SimpleEngine/Utility/HashUtils.h"


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

// --- 타입 리플렉션 테스트를 위한 Fixture 클래스 ---
class TypeReflectionTest : public ::testing::Test {};


// --- GetTypeName / GetFullTypeName 테스트 ---
// 각 SUBCASE를 독립적인 TEST_F로 분리하고, 테스트 목적에 맞는 이름을 부여합니다.

TEST_F(TypeReflectionTest, PrimitiveTypeNamesAreCorrect)
{
    using namespace se;

    // int와 그 변형(포인터, 참조 등)은 모두 기본 타입 이름 "int"를 반환해야 합니다.
    EXPECT_EQ(GetFullTypeName<int>(), "int");
    EXPECT_EQ(GetTypeName<int>(), "int");

    EXPECT_EQ(GetFullTypeName<const int[10]>(), "int");
    EXPECT_EQ(GetTypeName<const int[10]>(), "int");

    EXPECT_EQ(GetFullTypeName<const int*>(), "int");
    EXPECT_EQ(GetTypeName<const int*>(), "int");

    EXPECT_EQ(GetFullTypeName<const int* const>(), "int");
    EXPECT_EQ(GetTypeName<const int* const>(), "int");

    EXPECT_EQ(GetFullTypeName<const int* const&&>(), "int");
    EXPECT_EQ(GetTypeName<const int* const&&>(), "int");

    EXPECT_EQ(GetFullTypeName<const int&>(), "int");
    EXPECT_EQ(GetTypeName<const int&>(), "int");
}

TEST_F(TypeReflectionTest, ComplexPointerAndQualifierStripping)
{
    using namespace se;

    using Type1 = const volatile WeirdNamespace::MyEnum***** const volatile;
    EXPECT_EQ(GetFullTypeName<Type1>(), "WeirdNamespace::MyEnum");
    EXPECT_EQ(GetTypeName<Type1>(), "MyEnum");

    using Type2 = const volatile int* const* volatile** const;
    EXPECT_EQ(GetFullTypeName<Type2>(), "int");
    EXPECT_EQ(GetTypeName<Type2>(), "int");
}

TEST_F(TypeReflectionTest, ClassTypeNamesAreCorrect)
{
    using namespace se;

    using Type1 = const WeirdNamespace::MyClass* const&&;
    EXPECT_EQ(GetFullTypeName<Type1>(), "WeirdNamespace::MyClass");
    EXPECT_EQ(GetTypeName<Type1>(), "MyClass");

    using Type2 = WeirdNamespace::MyClass* const (&)[5];
    EXPECT_EQ(GetFullTypeName<Type2>(), "WeirdNamespace::MyClass");
    EXPECT_EQ(GetTypeName<Type2>(), "MyClass");
}

TEST_F(TypeReflectionTest, EnumInGlobalNamespace)
{
    using namespace se;

    using Type1 = TestEnum****** * **;
    // 전역 네임스페이스의 enum이므로 FullTypeName과 TypeName이 같아야 함
    EXPECT_EQ(GetFullTypeName<Type1>(), "TestEnum");
    EXPECT_EQ(GetTypeName<Type1>(), "TestEnum");
}


// --- TypeId 테스트 ---

TEST_F(TypeReflectionTest, TypeIdReturnsCorrectNameAndHash)
{
    using namespace se;

    constexpr TypeId TYPE_ID = TypeId::Of<int>();

    // 컴파일 타임 기능이지만, 런타임 값도 확인
    EXPECT_EQ(TYPE_ID.GetName(), "int");
    EXPECT_EQ(TYPE_ID.GetHash(), se::HashUtils::FNV("int"));

    // static_assert를 사용하여 컴파일 타임 검증도 명시
    static_assert(TYPE_ID.GetName() == "int");
    static_assert(TYPE_ID.GetHash() == se::HashUtils::FNV("int"));
    SUCCEED(); // static_assert가 통과했음을 gtest에 알림
}

TEST_F(TypeReflectionTest, TypeIdsForDifferentTypesAreDifferent)
{
    using namespace se;

    constexpr TypeId ID_INT = TypeId::Of<int>();
    constexpr TypeId ID_FLOAT = TypeId::Of<f32>();
    constexpr TypeId ID_CLASS = TypeId::Of<WeirdNamespace::MyClass>();

    EXPECT_NE(ID_INT, ID_FLOAT);
    EXPECT_NE(ID_INT, ID_CLASS);
    EXPECT_NE(ID_FLOAT, ID_CLASS);

    // 포인터/참조가 제거된 기본 타입은 같은 TypeId를 가져야 함
    constexpr TypeId ID_INT_PTR = TypeId::Of<int*>();
    constexpr TypeId ID_CONST_INT_REF = TypeId::Of<const int&>();
    EXPECT_NE(ID_INT, ID_INT_PTR);
    EXPECT_EQ(ID_INT, ID_CONST_INT_REF);
}
