#pragma once

#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"


// 에셋 payload를 다루는 테스트들이 함께 쓰는 값 생성 함수들입니다. 모든 필드를 기본값과 다르게 채웁니다.
namespace se::test_assets
{
/** value를 TOML 테이블로 씁니다. 에셋 타입에는 operator==가 없어서, 두 값이 같은지는 쓴 테이블로 비교합니다. */
template <typename T>
[[nodiscard]] toml::table WriteToml(const T& value)
{
    toml::table table;
    TomlWriter writer(table);
    EXPECT_TRUE(serde::Serialize(writer, value).HasValue());
    return table;
}

/** 테스트용 정점 하나를 만듭니다. seed마다 다른 값을 가집니다. */
[[nodiscard]] inline StaticVertex MakeVertex(const f32 seed)
{
    return {
        .position = { seed, seed + 1.0f, seed + 2.0f },
        .normal = { 0.0f, 1.0f, 0.0f },
        .tex_coord = { seed * 0.5f, 0.75f },
        .tangent = { 1.0f, 0.0f, 0.0f, -1.0f },
    };
}

/** 원점을 둘러싼 테스트용 바운딩 박스를 만듭니다. */
[[nodiscard]] inline AABBf MakeBounds(const f32 extent)
{
    return { Vector3f{ -extent, -extent * 2.0f, -extent * 3.0f }, Vector3f{ extent, extent * 2.0f, extent * 3.0f } };
}

/** 모든 필드를 기본값과 다르게 채운 StaticMesh를 만듭니다. */
[[nodiscard]] inline StaticMesh MakeStaticMesh()
{
    StaticMesh mesh;
    mesh.vertices = { MakeVertex(1.0f), MakeVertex(2.0f), MakeVertex(3.0f) };
    mesh.indices = { 0, 1, 2, 2, 1, 0 };
    mesh.lods = {
        MeshLOD{
            .screen_size = 0.5f,
            .sections = {
                MeshSection{ .index_offset = 0, .index_count = 3, .vertex_offset = -1, .vertex_count = 3, .material_slot = 1, .bounds = MakeBounds(1.0f) },
                MeshSection{ .index_offset = 3, .index_count = 3, .vertex_offset = 2, .vertex_count = 0, .material_slot = 0, .bounds = MakeBounds(0.5f) },
            },
        },
    };
    mesh.default_materials = { AssetId{ Guid::NewGuid() }, AssetId::invalid };
    mesh.bounds = MakeBounds(4.0f);
    return mesh;
}

/** 모든 필드를 기본값과 다르게 채운 SkeletalMesh를 만듭니다. */
[[nodiscard]] inline SkeletalMesh MakeSkeletalMesh()
{
    SkeletalMesh mesh;
    mesh.vertices = { MakeVertex(5.0f), MakeVertex(6.0f) };
    mesh.skin_vertices = {
        SkinVertex{ .bone_indices = { 0, 1, 2, 3 }, .bone_weights = { 0.5f, 0.25f, 0.125f, 0.125f } },
        SkinVertex{ .bone_indices = { 7, 0, 0, 0 }, .bone_weights = { 1.0f, 0.0f, 0.0f, 0.0f } },
    };
    mesh.indices = { 0, 1, 1 };
    mesh.bounds = MakeBounds(2.0f);
    return mesh;
}

/** 모든 필드를 기본값과 다르게 채운 Texture2D를 만듭니다. */
[[nodiscard]] inline Texture2D MakeTexture2D()
{
    Texture2D texture;
    texture.width = 4;
    texture.height = 2;
    texture.format = ETextureFormat::R8G8B8A8_UNORM_SRGB;
    texture.generate_mips = false;
    texture.mips = {
        MipDescriptor{ .offset = 0, .size = 32, .width = 4, .height = 2 },
        MipDescriptor{ .offset = 32, .size = 8, .width = 2, .height = 1 },
    };
    texture.pixels = { 0, 1, 2, 127, 128, 254, 255 };
    return texture;
}

/** 모든 필드를 기본값과 다르게 채운 Material을 만듭니다. */
[[nodiscard]] inline Material MakeMaterial()
{
    Material material;
    material.shader_program = "CoreShader://Test.hlsl";
    material.blend_mode = EBlendMode::Masked;
    material.shading_model = EShadingModel::Unlit;
    material.two_sided = true;
    material.alpha_cutoff = 0.25f;
    material.permutation_key = 7;
    material
        .AddParameter("BaseColor", EMaterialParamType::Float4, Vector4f{ 1.0f, 0.5f, 0.25f, 1.0f })
        .AddParameter("flags", EMaterialParamType::Uint);
    material.texture_slots = {
        MaterialTextureSlot{
            .name = "BaseColor",
            .fragment_slot = 2,
            .sampler = ESamplerType::PointClamp,
            .default_texture_id = AssetId{ Guid::NewGuid() },
        },
    };
    return material;
}

/** 텍스처 오버라이드 두 개와 설정된 Optional 두 개를 가진 MaterialInstance를 만듭니다. */
[[nodiscard]] inline MaterialInstance MakeMaterialInstance()
{
    MaterialInstance instance;
    instance.parent_material_id = AssetId{ Guid::NewGuid() };
    instance.parameter_values = { 0, 0, 128, 63, 1, 0, 0, 0 };
    instance.texture_overrides.Insert("BaseColor", AssetId{ Guid::NewGuid() });
    instance.texture_overrides.Insert("Normal", AssetId::invalid);
    instance.blend_mode_override = EBlendMode::Translucent;
    instance.two_sided_override = false;
    return instance;
}
} // namespace se::test_assets
