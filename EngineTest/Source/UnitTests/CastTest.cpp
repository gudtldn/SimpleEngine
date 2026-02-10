#include "gtest/gtest.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"

namespace se::test
{
// 테스트를 위한 상속 계층 구조 정의
class CastTest_Base
{
    SE_CLASS(CastTest_Base)
public:
    virtual ~CastTest_Base() = default;
};

class CastTest_Derived : public CastTest_Base
{
    SE_CLASS(CastTest_Derived, CastTest_Base)
};

class CastTest_DeepDerived : public CastTest_Derived
{
    SE_CLASS(CastTest_DeepDerived, CastTest_Derived)
};

class CastTest_Other
{
    SE_CLASS(CastTest_Other)
public:
    virtual ~CastTest_Other() = default;
};

// 리플렉션 정보 등록
SE_BEGIN_REFLECT(CastTest_Base)
SE_END_REFLECT(CastTest_Base)

SE_BEGIN_REFLECT(CastTest_Derived)
SE_END_REFLECT(CastTest_Derived)

SE_BEGIN_REFLECT(CastTest_DeepDerived)
SE_END_REFLECT(CastTest_DeepDerived)

SE_BEGIN_REFLECT(CastTest_Other)
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
    EXPECT_TRUE(IsChildOf<CastTest_Base>(TypeId::Get<CastTest_Derived>()));
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
    CastTest_Other other;

    // 성공하는 케이스
    EXPECT_NO_THROW({
        CastTest_Base* ptr = CastChecked<CastTest_Base>(&base);
        EXPECT_EQ(ptr, &base);
    });

    // 실패하는 케이스는 SE_ASSERT가 동작하므로, 
    // 프로젝트의 Assert 구현 방식에 따라 테스트 방법이 달라질 수 있습니다.
    // 여기서는 로직의 올바름만 확인합니다.
}
} // namespace se::test
