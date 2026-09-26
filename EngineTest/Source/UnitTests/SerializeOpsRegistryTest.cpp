#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Serialization/BuiltinTraits.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"


// 내장 트레이트가 SerializeOpsRegistry에 FORMAT_VERSION과 함께 연결되는지, 미등록 타입은 NullOpt인지 검증
namespace se_serialize_ops_registry_test
{
using namespace se;

/** 등록하지 않는 타입입니다. Find가 NullOpt를 돌려줘야 합니다. */
struct NotRegistered
{
    i32 value = 0;
};
} // namespace se_serialize_ops_registry_test


TEST(SerializeOpsRegistryTest, BuiltinStringIsInstalled)
{
    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<se::String>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_NE(ops->write, nullptr);
    EXPECT_NE(ops->read, nullptr);
    EXPECT_EQ(ops->format_version, 1u);
}

TEST(SerializeOpsRegistryTest, BuiltinStringNameIsInstalled)
{
    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<se::StringName>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_NE(ops->write, nullptr);
    EXPECT_NE(ops->read, nullptr);
    EXPECT_EQ(ops->format_version, 1u);
}

TEST(SerializeOpsRegistryTest, BuiltinGuidIsInstalled)
{
    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<se::Guid>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_NE(ops->write, nullptr);
    EXPECT_NE(ops->read, nullptr);
    EXPECT_EQ(ops->format_version, 1u);
}

TEST(SerializeOpsRegistryTest, BuiltinTypeIdIsInstalled)
{
    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<se::TypeId>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_NE(ops->write, nullptr);
    EXPECT_NE(ops->read, nullptr);
    EXPECT_EQ(ops->format_version, 1u);
}

TEST(SerializeOpsRegistryTest, BuiltinPathIsInstalled)
{
    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<se::Path>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_NE(ops->write, nullptr);
    EXPECT_NE(ops->read, nullptr);
    EXPECT_EQ(ops->format_version, 1u);
}

TEST(SerializeOpsRegistryTest, BuiltinVPathIsInstalled)
{
    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<se::VPath>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_NE(ops->write, nullptr);
    EXPECT_NE(ops->read, nullptr);
    EXPECT_EQ(ops->format_version, 1u);
}

TEST(SerializeOpsRegistryTest, BuiltinContentHashIsInstalled)
{
    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<se::ContentHash>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_NE(ops->write, nullptr);
    EXPECT_NE(ops->read, nullptr);
    EXPECT_EQ(ops->format_version, 1u);
}

TEST(SerializeOpsRegistryTest, UnregisteredTypeIsNullOpt)
{
    using namespace se_serialize_ops_registry_test;

    const auto ops = se::SerializeOpsRegistry::Get().Find(se::TypeId::Of<NotRegistered>());
    EXPECT_FALSE(ops.HasValue());
}
