#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/Registrar.h"

#include <cstddef>
#include <variant>


// 등록 매크로가 아직 없으므로, Registrar<T>::Fill() 을 손으로 작성해
// "전이 폐포 자동 등록" 메커니즘 자체를 검증합니다.
namespace se_registry_golden_test
{
struct Nested
{
    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct Root
{
    i32 value = 0;
    Nested nested;
    se::Array<i32> numbers;
};
} // namespace se_registry_golden_test

SE_DECLARE_REFLECTION(se_registry_golden_test::Nested)
SE_DECLARE_REFLECTION(se_registry_golden_test::Root)

namespace se
{
void Registrar<se_registry_golden_test::Nested>::Fill(TypeInfo& info)
{
    using T = se_registry_golden_test::Nested;
    info.size = sizeof(T);
    info.alignment = alignof(T);
    info.name = TypeNameOf<T>();

    EnsureRegistered<f32>();

    Array<FieldInfo>& fields = TypeRegistry::Get().EmplaceStructStorage(TypeId::Of<T>()).fields;
    fields.Push(FieldInfo{ .name = "x", .type = TypeId::Of<f32>(), .offset = offsetof(T, x) });
    fields.Push(FieldInfo{ .name = "y", .type = TypeId::Of<f32>(), .offset = offsetof(T, y) });

    info.shape = StructInfo{ .fields = fields };
}

void Registrar<se_registry_golden_test::Root>::Fill(TypeInfo& info)
{
    using T = se_registry_golden_test::Root;
    using se_registry_golden_test::Nested;

    info.size = sizeof(T);
    info.alignment = alignof(T);
    info.name = TypeNameOf<T>();

    // 필드 타입들을 등록합니다 — 이 호출들이 "전이 폐포"의 실체입니다.
    // 호출부(테스트 코드)는 Nested/i32/Array<i32> 를 직접 등록한 적이 없습니다.
    EnsureRegistered<i32>();
    EnsureRegistered<Nested>();
    EnsureRegistered<Array<i32>>();

    Array<FieldInfo>& fields = TypeRegistry::Get().EmplaceStructStorage(TypeId::Of<T>()).fields;
    fields.Push(FieldInfo{ .name = "value",   .type = TypeId::Of<i32>(),        .offset = offsetof(T, value) });
    fields.Push(FieldInfo{ .name = "nested",  .type = TypeId::Of<Nested>(),     .offset = offsetof(T, nested) });
    fields.Push(FieldInfo{ .name = "numbers", .type = TypeId::Of<Array<i32>>(), .offset = offsetof(T, numbers) });

    info.shape = StructInfo{ .fields = fields };
}
} // namespace se


TEST(TypeRegistryGoldenTest, TransitiveClosureRegistersFieldTypes)
{
    using namespace se_registry_golden_test;

    // Root 만 명시적으로 등록합니다. Nested/i32/Array<i32> 는 전이 폐포로 딸려와야 합니다.
    const se::TypeInfo& root_info = se::EnsureRegistered<Root>();

    EXPECT_EQ(root_info.id.Value(), se::TypeId::Of<Root>().Value());
    ASSERT_TRUE(std::holds_alternative<se::StructInfo>(root_info.shape));

    const auto& shape = std::get<se::StructInfo>(root_info.shape);
    ASSERT_EQ(shape.fields.Len(), 3u);
    EXPECT_EQ(shape.fields[0].name, "value");
    EXPECT_EQ(shape.fields[1].name, "nested");
    EXPECT_EQ(shape.fields[2].name, "numbers");
    EXPECT_EQ(shape.fields[1].offset, offsetof(Root, nested));

    // 호출부는 아래 타입들을 직접 등록한 적이 없습니다 — Root::Fill() 안에서 전이적으로 등록됐어야 합니다.
    EXPECT_TRUE(se::TypeRegistry::Get().Find(se::TypeId::Of<Nested>()).HasValue());
    EXPECT_TRUE(se::TypeRegistry::Get().Find(se::TypeId::Of<i32>()).HasValue());
    EXPECT_TRUE(se::TypeRegistry::Get().Find(se::TypeId::Of<se::Array<i32>>()).HasValue());
}

TEST(TypeRegistryGoldenTest, ArrayFieldShapeIsCorrect)
{
    using namespace se_registry_golden_test;
    se::EnsureRegistered<Root>();

    const se::TypeInfo& array_info = se::TypeRegistry::Get().FindChecked(se::TypeId::Of<se::Array<i32>>());
    ASSERT_TRUE(std::holds_alternative<se::ArrayInfo>(array_info.shape));
    EXPECT_EQ(std::get<se::ArrayInfo>(array_info.shape).element.Value(), se::TypeId::Of<i32>().Value());
}

TEST(TypeRegistryGoldenTest, EnsureRegisteredIsIdempotent)
{
    using namespace se_registry_golden_test;
    const se::TypeInfo& first = se::EnsureRegistered<Nested>();
    const se::TypeInfo& second = se::EnsureRegistered<Nested>();
    EXPECT_EQ(&first, &second); // function-local static — 항상 같은 객체를 반환해야 합니다.
}

TEST(TypeRegistryGoldenTest, FindNeverFailsForTransitivelyRegisteredTypes)
{
    using namespace se_registry_golden_test;
    se::EnsureRegistered<Root>();

    for (const se::TypeId id : { se::TypeId::Of<Root>(), se::TypeId::Of<Nested>(), se::TypeId::Of<i32>() })
    {
        EXPECT_TRUE(se::TypeRegistry::Get().Find(id).HasValue());
    }
}
