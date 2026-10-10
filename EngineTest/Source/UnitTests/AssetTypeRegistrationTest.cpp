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
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "TestAssetFactories.h"

#include <ranges>
#include <string>
#include <string_view>

using namespace se;
using namespace se::test_assets;

// EngineCore가 등록한 에셋 타입을 EngineCore 밖(이 실행 파일)에서 Binary와 TOML로 왕복할 수 있는지 확인
namespace
{
/** original을 Binary로 쓰고 기본값 객체에 다시 읽어, 읽은 값이 원본과 같은지 확인합니다. */
template <typename T>
void ExpectBinaryRoundTrip(const T& original)
{
    SCOPED_TRACE(std::string_view{ TypeNameOf<T>() });

    Array<u8> buffer;
    BinaryWriter writer(buffer);
    ASSERT_TRUE(serde::Serialize(writer, original).HasValue());

    BinaryReader reader(buffer);
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


TEST(AssetTypeRegistrationTest, EveryAssetTypeRoundTripsThroughBinary)
{
    ExpectBinaryRoundTrip(MakeStaticMesh());
    ExpectBinaryRoundTrip(MakeSkeletalMesh());
    ExpectBinaryRoundTrip(MakeTexture2D());
    ExpectBinaryRoundTrip(MakeMaterial());
    ExpectBinaryRoundTrip(MakeMaterialInstance());
}

TEST(AssetTypeRegistrationTest, EveryAssetTypeRoundTripsThroughToml)
{
    ExpectTomlRoundTrip(MakeStaticMesh());
    ExpectTomlRoundTrip(MakeSkeletalMesh());
    ExpectTomlRoundTrip(MakeTexture2D());
    ExpectTomlRoundTrip(MakeMaterial());
    ExpectTomlRoundTrip(MakeMaterialInstance());
}
