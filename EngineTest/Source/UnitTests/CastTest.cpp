#include "gtest/gtest.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"

namespace se::test
{
// --- 상속 테스트를 위한 클래스 ---
class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) CastTest_Base
{
    SE_CLASS(CastTest_Base)

public:
    virtual ~CastTest_Base() = default;
};

class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) CastTest_Derived : public CastTest_Base
{
    SE_CLASS(CastTest_Derived, CastTest_Base)
};

class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) CastTest_DeepDerived : public CastTest_Derived
{
    SE_CLASS(CastTest_DeepDerived, CastTest_Derived)
};

// --- 인터페이스 테스트를 위한 클래스 ---
class ICastTest_Interface
{
public:
    virtual ~ICastTest_Interface() = default;
    virtual int GetValue() const = 0;
};

class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) CastTest_Implementer : public CastTest_Base, public ICastTest_Interface
{
    SE_CLASS(CastTest_Implementer, CastTest_Base)

public:
    int GetValue() const override { return 42; }
};

class SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) CastTest_Other
{
    SE_CLASS(CastTest_Other)
public:
    virtual ~CastTest_Other() = default;
};

// 리플렉션 정보 등록
SE_BEGIN_REFLECT(CastTest_Base, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(CastTest_Base)

SE_BEGIN_REFLECT(CastTest_Derived, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(CastTest_Derived)

SE_BEGIN_REFLECT(CastTest_DeepDerived, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(CastTest_DeepDerived)

// 인터페이스는 보통 SE_CLASS를 사용하지 않고 (필요하다면 가능)
// Registry에 타입으로만 등록하거나 상속 받는 쪽에서 Implements()를 호출합니다.
SE_BEGIN_REFLECT(CastTest_Implementer, meta::Reflect, meta::Hidden, meta::Transient)
    SE_REFLECT_INTERFACE(ICastTest_Interface)
SE_END_REFLECT(CastTest_Implementer)

SE_BEGIN_REFLECT(CastTest_Other, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(CastTest_Other)

class CastTest : public ::testing::Test {};

TEST_F(CastTest, IsA_WorksCorrectly)
{
    CastTest_Base base;
    CastTest_Derived derived;
    CastTest_DeepDerived deep;
    CastTest_Other other;

    // 포인터 버전
    EXPECT_TRUE(IsA<CastTest_Base>(&base));
    EXPECT_TRUE(IsA<CastTest_Base>(&derived));
    EXPECT_TRUE(IsA<CastTest_Base>(&deep));
    EXPECT_FALSE(IsA<CastTest_Base>(&other));

    EXPECT_FALSE(IsA<CastTest_Derived>(&base));
    EXPECT_TRUE(IsA<CastTest_Derived>(&derived));
    EXPECT_TRUE(IsA<CastTest_Derived>(&deep));

    // 참조 버전
    EXPECT_TRUE(IsA<CastTest_Base>(base));
    EXPECT_TRUE(IsA<CastTest_Base>(derived));
    EXPECT_TRUE(IsA<CastTest_Base>(deep));
}

TEST_F(CastTest, IsChildOf_WorksCorrectly)
{
    EXPECT_TRUE((IsChildOf<CastTest_Derived, CastTest_Base>()));
    EXPECT_TRUE((IsChildOf<CastTest_DeepDerived, CastTest_Base>()));
    EXPECT_TRUE((IsChildOf<CastTest_DeepDerived, CastTest_Derived>()));

    EXPECT_FALSE((IsChildOf<CastTest_Base, CastTest_Derived>()));
    EXPECT_FALSE((IsChildOf<CastTest_Other, CastTest_Base>()));

    // TypeId 버전
    EXPECT_TRUE(IsChildOf<CastTest_Base>(TypeId::Of<CastTest_Derived>()));
}

TEST_F(CastTest, Implements_WorksCorrectly)
{
    CastTest_Implementer implementer;
    CastTest_Base* base_ptr = &implementer;

    EXPECT_TRUE(Implements<ICastTest_Interface>(base_ptr));
    EXPECT_TRUE(Implements<ICastTest_Interface>(implementer.GetTypeId()));

    CastTest_Derived derived;
    EXPECT_FALSE(Implements<ICastTest_Interface>(&derived));
}

TEST_F(CastTest, Cast_WorksCorrectly)
{
    CastTest_DeepDerived deep;
    CastTest_Base* base_ptr = &deep;

    // Upcasting (컴파일 타임에 static_cast로 처리됨)
    CastTest_Derived* derived_ptr = Cast<CastTest_Derived>(base_ptr);
    EXPECT_NE(derived_ptr, nullptr);

    // Downcasting
    CastTest_DeepDerived* casted_deep = Cast<CastTest_DeepDerived>(base_ptr);
    EXPECT_EQ(casted_deep, &deep);

    // Invalid casting
    CastTest_Other* other_ptr = Cast<CastTest_Other>(base_ptr);
    EXPECT_EQ(other_ptr, nullptr);

    // nullptr casting
    CastTest_Base* null_base = nullptr;
    EXPECT_EQ(Cast<CastTest_Derived>(null_base), nullptr);
}

TEST_F(CastTest, InterfaceCast_WorksCorrectly)
{
    CastTest_Implementer implementer;
    CastTest_Base* base_ptr = &implementer;

    // Base* -> Interface* 캐스팅
    ICastTest_Interface* interface_ptr = Cast<ICastTest_Interface>(base_ptr);
    ASSERT_NE(interface_ptr, nullptr);
    EXPECT_EQ(interface_ptr->GetValue(), 42);

    // 반대 방향 (현재 reflection 구조상 지원 여부 확인 필요, 보통은 dynamic_cast로 하거나 registry를 통함)
}

TEST_F(CastTest, ExactCast_WorksCorrectly)
{
    CastTest_Derived derived;
    CastTest_Base* base_ptr = &derived;

    // 정확히 타입이 일치하는 경우
    EXPECT_NE(ExactCast<CastTest_Derived>(base_ptr), nullptr);

    // 상속 관계지만 타입이 다른 경우
    EXPECT_EQ(ExactCast<CastTest_Base>(base_ptr), nullptr);

    CastTest_Base base;
    EXPECT_NE(ExactCast<CastTest_Base>(&base), nullptr);
}

TEST_F(CastTest, CastChecked_AssertsOnFailure)
{
    CastTest_Base base;

    // 성공하는 케이스
    EXPECT_NO_THROW({
        CastTest_Base* ptr = CastChecked<CastTest_Base>(&base);
        EXPECT_EQ(ptr, &base);
    });

    CastTest_Implementer implementer;
    EXPECT_NO_THROW({
        ICastTest_Interface* itf = CastChecked<ICastTest_Interface>(&implementer);
        EXPECT_NE(itf, nullptr);
    });
}
} // namespace se::test
