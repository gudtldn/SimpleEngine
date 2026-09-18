#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/Rtti.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


// 자체 RTTI(TypeRecord 기반) 캐스트가 언어 RTTI 없이 동작하는지 검증합니다.
// RGResourceBase 계층과 같은 모양(추상 루트 - 추상 중간 - 구체 말단)을 씁니다.
namespace se_runtime_cast_test
{
using namespace se;

/** 다형 루트. 구체 클래스가 SE_RTTI를 빠뜨리면 추상으로 남아 컴파일 에러가 납니다. */
class ResourceBase
{
public:
    SE_RTTI_ROOT()

    virtual ~ResourceBase() = default;
};

/** 추상 중간 계층 — 인스턴스화되지 않으므로 SE_RTTI가 필요 없습니다. */
class TextureBase : public ResourceBase
{
public:
    i32 width = 0;
};

class BufferBase : public ResourceBase
{
public:
    i32 size = 0;
};

class TransientTexture final : public TextureBase
{
public:
    SE_RTTI(TransientTexture)

    i32 pool_index = 0;
};

class ExternalTexture final : public TextureBase
{
public:
    SE_RTTI(ExternalTexture)
};

class TransientBuffer final : public BufferBase
{
public:
    SE_RTTI(TransientBuffer)
};

/** SubsystemBase + IUpdatable 패턴을 재현하는 인터페이스입니다. */
class ITickable
{
public:
    virtual ~ITickable() = default;
    virtual void Tick() {}
};

/** 두 번째 base(ITickable)는 오프셋이 0이 아닙니다. */
class TickableTexture final : public TextureBase, public ITickable
{
public:
    SE_RTTI(TickableTexture)
};

/** 가상 상속 없는 다이아몬드 — DiamondRoot가 두 서브오브젝트로 중복됩니다. */
class DiamondRoot
{
public:
    SE_RTTI_ROOT()

    virtual ~DiamondRoot() = default;

    i32 v = 0;
};

class DLeft : public DiamondRoot
{
public:
    i32 l = 0;
};

class DRight : public DiamondRoot
{
public:
    i32 r = 0;
};

class DBottom final : public DLeft, public DRight
{
public:
    SE_RTTI(DBottom)

    i32 b = 0;
};
} // namespace se_runtime_cast_test

SE_DECLARE_REFLECTION(se_runtime_cast_test::ResourceBase)
SE_DECLARE_REFLECTION(se_runtime_cast_test::TextureBase)
SE_DECLARE_REFLECTION(se_runtime_cast_test::BufferBase)
SE_DECLARE_REFLECTION(se_runtime_cast_test::TransientTexture)
SE_DECLARE_REFLECTION(se_runtime_cast_test::ExternalTexture)
SE_DECLARE_REFLECTION(se_runtime_cast_test::TransientBuffer)
SE_DECLARE_REFLECTION(se_runtime_cast_test::ITickable)
SE_DECLARE_REFLECTION(se_runtime_cast_test::TickableTexture)
SE_DECLARE_REFLECTION(se_runtime_cast_test::DiamondRoot)
SE_DECLARE_REFLECTION(se_runtime_cast_test::DLeft)
SE_DECLARE_REFLECTION(se_runtime_cast_test::DRight)
SE_DECLARE_REFLECTION(se_runtime_cast_test::DBottom)

SE_REFLECT_BEGIN(se_runtime_cast_test::ResourceBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::TextureBase)
    SE_BASE(se_runtime_cast_test::ResourceBase)
    SE_FIELD(width)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::BufferBase)
    SE_BASE(se_runtime_cast_test::ResourceBase)
    SE_FIELD(size)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::TransientTexture)
    SE_BASE(se_runtime_cast_test::TextureBase)
    SE_FIELD(pool_index)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::ExternalTexture)
    SE_BASE(se_runtime_cast_test::TextureBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::TransientBuffer)
    SE_BASE(se_runtime_cast_test::BufferBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::ITickable)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::TickableTexture)
    SE_BASE(se_runtime_cast_test::TextureBase)
    SE_BASE(se_runtime_cast_test::ITickable)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::DiamondRoot)
    SE_FIELD(v)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::DLeft)
    SE_BASE(se_runtime_cast_test::DiamondRoot)
    SE_FIELD(l)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::DRight)
    SE_BASE(se_runtime_cast_test::DiamondRoot)
    SE_FIELD(r)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se_runtime_cast_test::DBottom)
    SE_BASE(se_runtime_cast_test::DLeft)
    SE_BASE(se_runtime_cast_test::DRight)
    SE_FIELD(b)
SE_REFLECT_END()


TEST(RuntimeCastTest, DowncastToConcreteTypeSucceeds)
{
    using namespace se_runtime_cast_test;

    TransientTexture texture;
    ResourceBase* base = &texture;

    TransientTexture* result = se::Cast<TransientTexture>(base);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, &texture);
}

TEST(RuntimeCastTest, DowncastToIntermediateTypeSucceeds)
{
    using namespace se_runtime_cast_test;

    TransientTexture texture;
    ResourceBase* base = &texture;

    TextureBase* result = se::Cast<TextureBase>(base);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, static_cast<TextureBase*>(&texture));
}

TEST(RuntimeCastTest, CastToUnrelatedSiblingFails)
{
    using namespace se_runtime_cast_test;

    TransientTexture texture;
    ResourceBase* base = &texture;

    EXPECT_EQ(se::Cast<BufferBase>(base), nullptr);
    EXPECT_EQ(se::Cast<TransientBuffer>(base), nullptr);
    EXPECT_EQ(se::Cast<ExternalTexture>(base), nullptr);
}

TEST(RuntimeCastTest, CrossCastToInterfaceAdjustsPointer)
{
    using namespace se_runtime_cast_test;

    TickableTexture texture;
    ResourceBase* base = &texture;

    ITickable* tickable = se::Cast<ITickable>(base);
    ASSERT_NE(tickable, nullptr);
    EXPECT_EQ(tickable, static_cast<ITickable*>(&texture));

    // 두 번째 base이므로 포인터 조정이 실제로 일어나야 합니다.
    EXPECT_NE(static_cast<void*>(tickable), static_cast<void*>(&texture));
}

TEST(RuntimeCastTest, NullPointerCastsToNull)
{
    using namespace se_runtime_cast_test;

    ResourceBase* base = nullptr;
    EXPECT_EQ(se::Cast<TransientTexture>(base), nullptr);
    EXPECT_EQ(se::ExactCast<TransientTexture>(base), nullptr);
    EXPECT_FALSE(se::IsA<TransientTexture>(base));
}

TEST(RuntimeCastTest, ConstOverloadPreservesConstness)
{
    using namespace se_runtime_cast_test;

    const TransientTexture texture;
    const ResourceBase* base = &texture;

    const TransientTexture* result = se::Cast<TransientTexture>(base);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, &texture);
}

TEST(RuntimeCastTest, CastCheckedReturnsSamePointerOnSuccess)
{
    using namespace se_runtime_cast_test;

    TransientTexture texture;
    ResourceBase* base = &texture;

    EXPECT_EQ(se::CastChecked<TransientTexture>(base), &texture);
}

TEST(RuntimeCastTest, ExactCastMatchesOnlyTheDynamicType)
{
    using namespace se_runtime_cast_test;

    TransientTexture texture;
    ResourceBase* base = &texture;

    EXPECT_NE(se::ExactCast<TransientTexture>(base), nullptr);

    // 상속 체인을 타지 않으므로 중간 계층에는 매칭되지 않습니다.
    EXPECT_EQ(se::ExactCast<TextureBase>(base), nullptr);
    EXPECT_EQ(se::ExactCast<ResourceBase>(base), nullptr);
}

TEST(RuntimeCastTest, IsAFollowsInheritanceChain)
{
    using namespace se_runtime_cast_test;

    TransientTexture texture;
    ResourceBase* base = &texture;

    EXPECT_TRUE(se::IsA<TransientTexture>(base));
    EXPECT_TRUE(se::IsA<TextureBase>(base));
    EXPECT_TRUE(se::IsA<ResourceBase>(base));
    EXPECT_FALSE(se::IsA<BufferBase>(base));
}

TEST(RuntimeCastTest, DiamondUnambiguousBranchesStillCast)
{
    using namespace se_runtime_cast_test;

    DBottom bottom;
    DLeft* left = &bottom;

    // DLeft는 한 번만 나오므로 완전 객체를 특정할 수 있습니다.
    DRight* right = se::Cast<DRight>(left);
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right, static_cast<DRight*>(&bottom));
}

TEST(RuntimeCastTest, DiamondAmbiguousSourceReturnsNull)
{
    using namespace se_runtime_cast_test;

    DBottom bottom;
    DiamondRoot* root = static_cast<DLeft*>(&bottom);

    // DiamondRoot가 all_bases에 두 번 나오므로 완전 객체를 특정할 수 없습니다.
    EXPECT_EQ(se::Cast<DBottom>(root), nullptr);
    EXPECT_EQ(se::Cast<DRight>(root), nullptr);

    // 존재 여부만 묻는 IsA는 모호성과 무관하게 true입니다.
    EXPECT_TRUE(se::IsA<DBottom>(root));
}
