#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"
#include "SimpleEngine/ECS/ECSRegistry.h"
#include "SimpleEngine/ECS/World.h"


// ecs::Component 어노테이션이 붙은 타입이 등록될 때 ECSRegistry에 들어가는지 검증합니다.
namespace se_ecs_annotations_test
{
struct AnnotatedComponent
{
    i32 value = 0;
};
} // namespace se_ecs_annotations_test

SE_DECLARE_REFLECTION(se_ecs_annotations_test::AnnotatedComponent)

SE_REFLECT_BEGIN(se_ecs_annotations_test::AnnotatedComponent, se::ecs::Component)
    SE_FIELD(value)
SE_REFLECT_END()


TEST(ECSAnnotationsTest, ComponentAnnotationRegistersComponentOps)
{
    using namespace se_ecs_annotations_test;

    const auto ops = se::ECSRegistry::Get().GetComponentOps(se::TypeId::Of<AnnotatedComponent>());
    ASSERT_TRUE(ops.HasValue());
    EXPECT_EQ(ops->type, se::TypeId::Of<AnnotatedComponent>());

    se::World world;
    const se::Entity entity = world.SpawnEntity();
    ops->add_component(world, entity);
    EXPECT_TRUE(ops->has_component(world, entity));
    EXPECT_TRUE(world.HasComponent<AnnotatedComponent>(entity));
}
