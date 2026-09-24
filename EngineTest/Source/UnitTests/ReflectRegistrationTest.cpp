#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


// EnsureRegistered<T>()가 재귀·상호 재귀 타입, 정규 이름이 같은 타입, 정체성 충돌을
// 올바르게 처리하는지 검증합니다.
namespace se_reflect_registration_test
{
using namespace se;

struct RecursiveNode
{
    i32 value = 0;
    Array<RecursiveNode> children;
};

struct MutualB;

struct MutualA
{
    Array<MutualB> bs;
};

struct MutualB
{
    Array<MutualA> as;
};

// SE_REFLECT_BEGIN 타입은 정적 초기화에서 등록되므로, 충돌시키면 테스트 바이너리 전체가 시작하자마자 종료됩니다.
// enum은 매크로 없이 처음 쓸 때 등록되므로 death test 안에서만 충돌시킬 수 있습니다.
enum class CollideSmall : u8 {};
enum class CollideLarge : u32 {};
} // namespace se_reflect_registration_test

SE_DECLARE_REFLECTION(se_reflect_registration_test::RecursiveNode)
SE_DECLARE_REFLECTION(se_reflect_registration_test::MutualA)
SE_DECLARE_REFLECTION(se_reflect_registration_test::MutualB)

// 크기가 다른 두 enum에 같은 이름을 강제로 부여해, 정체성 충돌을 재현합니다.
SE_TYPE_NAME(se_reflect_registration_test::CollideSmall, "ReflectRegistrationTest_CollideSharedName");
SE_TYPE_NAME(se_reflect_registration_test::CollideLarge, "ReflectRegistrationTest_CollideSharedName");

SE_REFLECT_BEGIN(se_reflect_registration_test::RecursiveNode)
    SE_FIELD(value)
    SE_FIELD(children)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_reflect_registration_test::MutualA)
    SE_FIELD(bs)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_reflect_registration_test::MutualB)
    SE_FIELD(as)
SE_REFLECT_END()


TEST(ReflectRegistrationTest, SelfRecursiveTypeRegistersWithoutDeadlock)
{
    using namespace se_reflect_registration_test;

    const se::TypeInfo& first = se::EnsureRegistered<RecursiveNode>();
    const se::TypeInfo& second = se::EnsureRegistered<RecursiveNode>();
    EXPECT_EQ(&first, &second);

    const se::TypeInfo& array_info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<se::Array<RecursiveNode>>());
    const se::Optional<const se::ArrayInfo&> array_shape = array_info.AsArray();
    ASSERT_TRUE(array_shape.HasValue());
    EXPECT_EQ(array_shape->element.Value(), se::TypeId::Of<RecursiveNode>().Value());
}

TEST(ReflectRegistrationTest, MutuallyRecursiveTypesBothRegister)
{
    using namespace se_reflect_registration_test;

    const se::TypeInfo& info_a = se::EnsureRegistered<MutualA>();
    const se::TypeInfo& info_b = se::EnsureRegistered<MutualB>();
    EXPECT_EQ(info_a.id.Value(), se::TypeId::Of<MutualA>().Value());
    EXPECT_EQ(info_b.id.Value(), se::TypeId::Of<MutualB>().Value());
}

TEST(ReflectRegistrationTest, NormalizedIntegerAliasesShareOneSlot)
{
    // 정규 이름은 정수를 폭과 부호로만 구분하므로, long이 32비트이면 long과 i32의 TypeId가 같습니다.
    // 나중에 등록하는 쪽은 새 슬롯을 만들지 않고 기존 슬롯을 받아야 합니다.
    if constexpr (sizeof(long) == sizeof(i32))
    {
        const se::TypeInfo& as_long = se::EnsureRegistered<long>();
        const se::TypeInfo& as_i32 = se::EnsureRegistered<i32>();
        EXPECT_EQ(&as_long, &as_i32);
    }
    else
    {
        GTEST_SKIP() << "이 플랫폼에서는 long과 i32의 크기가 달라 검증 대상이 아닙니다.";
    }
}

TEST(ReflectRegistrationTest, IdentityMismatchUnderSharedTypeIdAborts)
{
    using namespace se_reflect_registration_test;

    EXPECT_DEATH(
        {
            se::EnsureRegistered<CollideSmall>();
            se::EnsureRegistered<CollideLarge>();
        },
        "TypeId collision");
}
