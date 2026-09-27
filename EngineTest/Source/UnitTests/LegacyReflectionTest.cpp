#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/String.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"


using namespace se;

namespace legacy_reflection_test
{
struct SE_ANNOTATION(=meta::Reflect, =meta::Hidden) SimpleData
{
    SE_ANNOTATION(=meta::Reflect)
    i32 x = 0;

    SE_ANNOTATION(=meta::Reflect)
    String name;
};

struct SE_ANNOTATION(=meta::Reflect, =meta::Hidden) BaseData
{
    SE_ANNOTATION(=meta::Reflect)
    i32 base_val = 0;
};

struct SE_ANNOTATION(=meta::Reflect, =meta::Hidden) DerivedData : BaseData
{
    using Super = BaseData;

    SE_ANNOTATION(=meta::Reflect)
    f32 derived_val = 0.0f;
};

enum class ETestColor : u8
{
    Red = 0,
    Green = 1,
    Blue = 2,
};
} // namespace legacy_reflection_test

using namespace legacy_reflection_test;

SE_DECLARE_REFLECTION_V1(SimpleData)
SE_BEGIN_REFLECT_V1(SimpleData, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(x, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(name, meta::Reflect)
SE_END_REFLECT_V1(SimpleData)

SE_DECLARE_REFLECTION_V1(BaseData)
SE_BEGIN_REFLECT_V1(BaseData, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(base_val, meta::Reflect)
SE_END_REFLECT_V1(BaseData)

SE_DECLARE_REFLECTION_V1(DerivedData)
SE_BEGIN_REFLECT_V1(DerivedData, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(derived_val, meta::Reflect)
SE_END_REFLECT_V1(DerivedData)

SE_REFLECT_ENUM_V1(ETestColor)

// 구조체와 열거형이 올바른 종류로 등록되는지 검증
TEST(LegacyReflectionTest, TypeInfo_CorrectKind)
{
    const TypeInfo_v1& struct_info = TypeRegistry_v1::Get().FindChecked(TypeId_v1::Of<SimpleData>());
    EXPECT_EQ(struct_info.kind, ETypeKind_v1::Struct);
    EXPECT_EQ(struct_info.properties.Len(), 2u);

    const TypeInfo_v1& enum_info = TypeRegistry_v1::Get().FindChecked(TypeId_v1::Of<ETestColor>());
    EXPECT_EQ(enum_info.kind, ETypeKind_v1::Enum);
    EXPECT_NE(enum_info.enum_entries, nullptr);
}

// Super로 선언한 부모가 bases에 등록되는지 검증
TEST(LegacyReflectionTest, Inheritance_BaseIdSet)
{
    const TypeInfo_v1& info = TypeRegistry_v1::Get().FindChecked(TypeId_v1::Of<DerivedData>());
    ASSERT_FALSE(info.bases.IsEmpty());
    EXPECT_EQ(info.bases[0].base_id, TypeId_v1::Of<BaseData>());
}
