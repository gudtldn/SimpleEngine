#include "gtest/gtest.h"

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
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"
#include "TestAssetFactories.h"

#include <ranges>
#include <string>
#include <string_view>

using namespace se;
using namespace se::test_assets;

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
