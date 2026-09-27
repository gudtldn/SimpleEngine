#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"

#include <ranges>
#include <string>
#include <string_view>

using namespace se;

// 에셋 payload 타입은 에셋 생성과 에디터 UI가 쓰는 레거시 리플렉션(TypeRegistry_v1)과 새 직렬화가 쓰는 리플렉션(TypeRegistry)에
// 모두 등록되므로, 한쪽에만 필드나 enum 값을 추가하는 실수를 잡기 위해 두 등록을 비교
// EngineCore가 등록한 에셋 타입을 EngineCore 밖(이 실행 파일)에서 Packed와 TOML로 왕복할 수 있는지도 확인
namespace
{
/** 타입 하나를 두 레지스트리에서 찾을 TypeId */
struct DualTypeIds
{
    TypeId id;
    TypeId_v1 legacy_id;
};

/** T의 두 TypeId를 만듭니다. 둘 다 타입 이름의 해시라 등록 템플릿을 인스턴스화하지 않습니다. */
template <typename T>
[[nodiscard]] DualTypeIds IdsOf()
{
    return { .id = TypeId::Of<T>(), .legacy_id = TypeId_v1::Of<T>() };
}

/** 이름들을 ", "로 이어 붙입니다. 실패 메시지에 두 목록이 그대로 보이도록 문자열로 비교합니다. */
template <std::ranges::input_range Names>
[[nodiscard]] std::string JoinNames(Names&& names)
{
    std::string joined;
    for (const StringView name : names)
    {
        if (!joined.empty())
        {
            joined += ", ";
        }
        joined += std::string_view{ name };
    }
    return joined;
}

/** value를 TOML 테이블로 씁니다. 에셋 타입에는 operator==가 없어서, 두 값이 같은지는 쓴 테이블로 비교합니다. */
template <typename T>
[[nodiscard]] toml::table WriteToml(const T& value)
{
    toml::table table;
    TomlWriter writer(table);
    EXPECT_TRUE(serde::Serialize(writer, value).HasValue());
    return table;
}

/** original을 Packed로 쓰고 기본값 객체에 다시 읽어, 읽은 값이 원본과 같은지 확인합니다. */
template <typename T>
void ExpectPackedRoundTrip(const T& original)
{
    SCOPED_TRACE(std::string_view{ TypeNameOf<T>() });

    Array<u8> buffer;
    PackedWriter writer(buffer);
    ASSERT_TRUE(serde::Serialize(writer, original).HasValue());

    PackedReader reader(buffer);
    T result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(WriteToml(result), WriteToml(original));
}

/** original을 TOML로 쓰고 기본값 객체에 다시 읽어, 읽은 값이 원본과 같은지 확인합니다. */
template <typename T>
void ExpectTomlRoundTrip(const T& original)
{
    SCOPED_TRACE(std::string_view{ TypeNameOf<T>() });

    const toml::table table = WriteToml(original);

    TomlReader reader(table);
    T result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(WriteToml(result), table);
}

/** 테스트용 정점 하나를 만듭니다. seed마다 다른 값을 가집니다. */
[[nodiscard]] StaticVertex MakeVertex(const f32 seed)
{
    return {
        .position = { seed, seed + 1.0f, seed + 2.0f },
        .normal = { 0.0f, 1.0f, 0.0f },
        .tex_coord = { seed * 0.5f, 0.75f },
        .tangent = { 1.0f, 0.0f, 0.0f, -1.0f },
    };
}

/** 원점을 둘러싼 테스트용 바운딩 박스를 만듭니다. */
[[nodiscard]] AABBf MakeBounds(const f32 extent)
{
    return { Vector3f{ -extent, -extent * 2.0f, -extent * 3.0f }, Vector3f{ extent, extent * 2.0f, extent * 3.0f } };
}

/** 모든 필드를 기본값과 다르게 채운 StaticMesh를 만듭니다. */
[[nodiscard]] StaticMesh MakeStaticMesh()
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
[[nodiscard]] SkeletalMesh MakeSkeletalMesh()
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
[[nodiscard]] Texture2D MakeTexture2D()
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
[[nodiscard]] Material MakeMaterial()
{
    Material material;
    material.vertex_shader = "CoreShader://Test.vert";
    material.fragment_shader = "CoreShader://Test.frag";
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
[[nodiscard]] MaterialInstance MakeMaterialInstance()
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
} // namespace


TEST(AssetTypeRegistrationTest, BothRegistrationsListSameFields)
{
    const DualTypeIds asset_types[] = {
        IdsOf<AssetBase>(),
        IdsOf<StaticMesh>(),
        IdsOf<SkeletalMesh>(),
        IdsOf<MipDescriptor>(),
        IdsOf<Texture2D>(),
        IdsOf<MaterialParameterDescriptor>(),
        IdsOf<MaterialTextureSlot>(),
        IdsOf<Material>(),
        IdsOf<MaterialInstance>(),
    };

    for (const DualTypeIds& type : asset_types)
    {
        const auto legacy_info = TypeRegistry_v1::Get().Find(type.legacy_id);
        ASSERT_TRUE(legacy_info.HasValue());
        SCOPED_TRACE(std::string_view{ legacy_info->name });

        const auto info = TypeRegistry::Get().Find(type.id);
        ASSERT_TRUE(info.HasValue()) << "The type is not registered with SE_REFLECT_BEGIN.";
        const auto struct_info = info->AsStruct();
        ASSERT_TRUE(struct_info.HasValue());

        // 이름과 순서가 모두 같아야 함
        EXPECT_EQ(
            JoinNames(struct_info->fields | std::views::transform(&FieldInfo::name)),
            JoinNames(legacy_info->properties | std::views::transform(&PropertyInfo_v1::name))
        );
    }
}

TEST(AssetTypeRegistrationTest, BothRegistrationsListSameEnumNames)
{
    const DualTypeIds enum_types[] = {
        IdsOf<ETextureFormat>(),
        IdsOf<EBlendMode>(),
        IdsOf<EShadingModel>(),
        IdsOf<EMaterialParamType>(),
        IdsOf<ESamplerType>(),
    };

    for (const DualTypeIds& type : enum_types)
    {
        const auto legacy_info = TypeRegistry_v1::Get().Find(type.legacy_id);
        ASSERT_TRUE(legacy_info.HasValue());
        ASSERT_NE(legacy_info->enum_entries, nullptr);
        SCOPED_TRACE(std::string_view{ legacy_info->name });

        const EnumEntry_v1* legacy_data = nullptr;
        usize legacy_count = 0;
        legacy_info->enum_entries(legacy_data, legacy_count);
        const ArrayView<const EnumEntry_v1> legacy_entries(legacy_data, legacy_count);

        const auto info = TypeRegistry::Get().Find(type.id);
        ASSERT_TRUE(info.HasValue()) << "The enum is not registered with SE_REFLECT_ENUM_BEGIN.";
        const auto enum_info = info->AsEnum();
        ASSERT_TRUE(enum_info.HasValue());

        // 레거시는 값을 자동으로 모으고 SE_REFLECT_ENUM_BEGIN은 SE_ENUM_VALUE로 적은 것만 가지므로, 값을 추가할 때 빠뜨리면 실패
        EXPECT_EQ(
            JoinNames(enum_info->entries | std::views::transform(&EnumEntry::name)),
            JoinNames(legacy_entries | std::views::transform(&EnumEntry_v1::name))
        );
    }
}

TEST(AssetTypeRegistrationTest, EveryAssetTypeRoundTripsThroughPacked)
{
    ExpectPackedRoundTrip(MakeStaticMesh());
    ExpectPackedRoundTrip(MakeSkeletalMesh());
    ExpectPackedRoundTrip(MakeTexture2D());
    ExpectPackedRoundTrip(MakeMaterial());
    ExpectPackedRoundTrip(MakeMaterialInstance());
}

TEST(AssetTypeRegistrationTest, EveryAssetTypeRoundTripsThroughToml)
{
    ExpectTomlRoundTrip(MakeStaticMesh());
    ExpectTomlRoundTrip(MakeSkeletalMesh());
    ExpectTomlRoundTrip(MakeTexture2D());
    ExpectTomlRoundTrip(MakeMaterial());
    ExpectTomlRoundTrip(MakeMaterialInstance());
}
