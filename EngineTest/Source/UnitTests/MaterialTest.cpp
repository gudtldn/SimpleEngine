#include "gtest/gtest.h"

#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "SimpleEngine/Asset/BuiltinAssets.h"

using namespace se;

namespace
{
AssetId NewId()
{
    return AssetId(Guid::NewGuid());
}

class MaterialTest : public ::testing::Test {};
}


TEST_F(MaterialTest, ParameterAlignment_std140)
{
    Material mat;

    // 1. Float (4 bytes, align 4) -> Offset 0
    mat.AddParameter("Param1", EMaterialParamType::Float);
    // 2. Float2 (8 bytes, align 8) -> Offset 8 (not 4)
    mat.AddParameter("Param2", EMaterialParamType::Float2);
    // 3. Float3 (12 bytes, align 16) -> Offset 16 (not 12)
    mat.AddParameter("Param3", EMaterialParamType::Float3);
    // 4. Float (4 bytes, align 4) -> Offset 28 (16 + 12 = 28, align 4 is ok)
    mat.AddParameter("Param4", EMaterialParamType::Float);
    // 5. Float4 (16 bytes, align 16) -> Offset 32 (not 28)
    mat.AddParameter("Param5", EMaterialParamType::Float4);

    auto p1 = mat.FindParameter("Param1");
    auto p2 = mat.FindParameter("Param2");
    auto p3 = mat.FindParameter("Param3");
    auto p4 = mat.FindParameter("Param4");
    auto p5 = mat.FindParameter("Param5");

    ASSERT_TRUE(p1.HasValue());
    ASSERT_TRUE(p2.HasValue());
    ASSERT_TRUE(p3.HasValue());
    ASSERT_TRUE(p4.HasValue());
    ASSERT_TRUE(p5.HasValue());

    EXPECT_EQ(p1->offset, 0);
    EXPECT_EQ(p2->offset, 8);
    EXPECT_EQ(p3->offset, 16);
    EXPECT_EQ(p4->offset, 28);
    EXPECT_EQ(p5->offset, 32);

    // 전체 블록 크기 체크 (마지막 32 + 16 = 48, 16바이트 정렬되어 있음)
    EXPECT_EQ(mat.ComputeParameterBlockSize(), 48);
}

TEST_F(MaterialTest, FinalizeLayout_DefaultValues)
{
    Material mat;
    mat.AddParameter("Color", EMaterialParamType::Float4, { 1.0f, 0.5f, 0.0f, 1.0f });
    mat.AddParameter("Intensity", EMaterialParamType::Float, { 2.0f, 0.0f, 0.0f, 0.0f });
    mat.FinalizeLayout();

    const auto& block = mat.GetDefaultParameterBlock();
    ASSERT_EQ(block.Len(), 32); // Float4(16) + Float(4) -> 20 -> align 16 -> 32

    float values[8];
    std::memcpy(values, block.Data(), 32);

    EXPECT_FLOAT_EQ(values[0], 1.0f);
    EXPECT_FLOAT_EQ(values[1], 0.5f);
    EXPECT_FLOAT_EQ(values[2], 0.0f);
    EXPECT_FLOAT_EQ(values[3], 1.0f);
    EXPECT_FLOAT_EQ(values[4], 2.0f);
}

TEST_F(MaterialTest, MaterialInstance_Inheritance)
{
    Material mat;
    mat.AddParameter("BaseColor", EMaterialParamType::Float4, { 1.0f, 1.0f, 1.0f, 1.0f });
    mat.FinalizeLayout();

    MaterialInstance inst;
    inst.InitializeFromParent(mat);

    ASSERT_EQ(inst.GetParameterBytes().Len(), mat.GetDefaultParameterBlock().Len());
    EXPECT_EQ(std::memcmp(inst.GetParameterBytes().Data(), mat.GetDefaultParameterBlock().Data(), inst.GetParameterBytes().Len()), 0);

    // 텍스처 기본값 로직 체크
    const AssetId def_tex = NewId();
    mat.texture_slots.Push({ .name = "Albedo", .default_texture_id = def_tex });

    // 오버라이드 없을 때
    EXPECT_EQ(inst.GetTextureOrDefault("Albedo", mat), def_tex);

    // 오버라이드 있을 때
    const AssetId override_tex = NewId();
    inst.texture_overrides.Insert("Albedo", override_tex);
    EXPECT_EQ(inst.GetTextureOrDefault("Albedo", mat), override_tex);

    // 존재하지 않는 슬롯
    EXPECT_EQ(inst.GetTextureOrDefault("Invalid", mat), AssetId::Invalid);
}

TEST_F(MaterialTest, MaterialInstance_ChangeParent)
{
    // Parent 1: Float4, Float
    Material mat1;
    mat1.AddParameter("P1_A", EMaterialParamType::Float4, { 1.0f, 1.0f, 1.0f, 1.0f });
    mat1.AddParameter("P1_B", EMaterialParamType::Float, { 0.5f, 0.0f, 0.0f, 0.0f });
    mat1.FinalizeLayout();

    // Parent 2: Float3
    Material mat2;
    mat2.AddParameter("P2_A", EMaterialParamType::Float3, { 0.1f, 0.2f, 0.3f, 0.0f });
    mat2.FinalizeLayout();

    MaterialInstance inst;

    // Initial: Mat1
    inst.InitializeFromParent(mat1);
    EXPECT_EQ(inst.GetParameterBytes().Len(), 32);
    float val1[2];
    std::memcpy(&val1[0], inst.GetParameterBytes().Data(), 4);      // P1_A.x
    std::memcpy(&val1[1], inst.GetParameterBytes().Data() + 16, 4); // P1_B
    EXPECT_FLOAT_EQ(val1[0], 1.0f);
    EXPECT_FLOAT_EQ(val1[1], 0.5f);

    // Change to Mat2
    inst.InitializeFromParent(mat2);
    EXPECT_EQ(inst.GetParameterBytes().Len(), 16);
    float val2[3];
    std::memcpy(val2, inst.GetParameterBytes().Data(), 12);
    EXPECT_FLOAT_EQ(val2[0], 0.1f);
    EXPECT_FLOAT_EQ(val2[1], 0.2f);
    EXPECT_FLOAT_EQ(val2[2], 0.3f);
}

TEST_F(MaterialTest, TextureOverrideIndependence)
{
    Material mat;
    const AssetId def1 = NewId();
    const AssetId def2 = NewId();
    mat.texture_slots.Push({ .name = "Tex1", .default_texture_id = def1 });
    mat.texture_slots.Push({ .name = "Tex2", .default_texture_id = def2 });

    MaterialInstance inst;

    // Override only Tex1
    const AssetId over1 = NewId();
    inst.texture_overrides.Insert("Tex1", over1);

    EXPECT_EQ(inst.GetTextureOrDefault("Tex1", mat), over1);
    EXPECT_EQ(inst.GetTextureOrDefault("Tex2", mat), def2); // Tex2 should still be default
}

TEST_F(MaterialTest, EmptyMaterial)
{
    Material mat;
    mat.FinalizeLayout();
    EXPECT_EQ(mat.ComputeParameterBlockSize(), 0);
    EXPECT_EQ(mat.GetDefaultParameterBlock().Len(), 0);

    MaterialInstance inst;
    inst.InitializeFromParent(mat);
    EXPECT_EQ(inst.GetParameterBytes().Len(), 0);
}

TEST_F(MaterialTest, Float3AtEndAlignment)
{
    Material mat;
    mat.AddParameter("Param", EMaterialParamType::Float3);
    mat.FinalizeLayout();

    // Float3 size is 12, but block must be aligned to 16
    EXPECT_EQ(mat.ComputeParameterBlockSize(), 16);
    EXPECT_EQ(mat.GetDefaultParameterBlock().Len(), 16);
}

TEST_F(MaterialTest, SearchFunctions)
{
    Material mat;
    mat.AddParameter("MyParam", EMaterialParamType::Float);
    mat.texture_slots.Push({ .name = "MySlot" });

    EXPECT_TRUE(mat.FindParameter("MyParam").HasValue());
    EXPECT_FALSE(mat.FindParameter("Wrong").HasValue());

    EXPECT_TRUE(mat.FindTextureSlot("MySlot").HasValue());
    EXPECT_FALSE(mat.FindTextureSlot("Wrong").HasValue());
}
