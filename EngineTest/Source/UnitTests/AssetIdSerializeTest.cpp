#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Types/Guid.h"

#include <string>

using namespace se;

// AssetId가 struct 필드와 배열 원소에서 Guid와 같은 표현(텍스트는 GUID 문자열, 바이너리는 16바이트)으로 왕복하는지 검증
namespace se_asset_id_serialize_test
{
/** AssetId를 struct 필드와 배열 원소로 담는 타입 */
struct HasAssetIds
{
    AssetId id;
    Array<AssetId> ids;

    [[nodiscard]] bool operator==(const HasAssetIds&) const = default;
};
} // namespace se_asset_id_serialize_test

SE_DECLARE_REFLECTION(se_asset_id_serialize_test::HasAssetIds)

SE_REFLECT_BEGIN(se_asset_id_serialize_test::HasAssetIds)
    SE_FIELD(id)
    SE_FIELD(ids)
SE_REFLECT_END()


namespace
{
/** 필드가 유효한 경우와 무효인 경우의 원본. 배열에는 유효한 값과 무효 값을 섞어 담습니다. */
[[nodiscard]] Array<se_asset_id_serialize_test::HasAssetIds> MakeOriginals()
{
    using se_asset_id_serialize_test::HasAssetIds;
    return {
        HasAssetIds{ .id = AssetId{ Guid::NewGuid() }, .ids = { AssetId{ Guid::NewGuid() }, AssetId::invalid } },
        HasAssetIds{ .id = AssetId::invalid, .ids = { AssetId::invalid, AssetId{ Guid::NewGuid() } } },
    };
}

/** 읽을 대상. 무효 AssetId가 기본값이 아니라 데이터에서 읽은 값인지 구분되도록 유효한 값으로 채워 둡니다. */
[[nodiscard]] se_asset_id_serialize_test::HasAssetIds MakeTarget()
{
    return { .id = AssetId{ Guid::NewGuid() }, .ids = { AssetId{ Guid::NewGuid() } } };
}
} // namespace


TEST(AssetIdSerializeTest, PackedRoundTrip)
{
    using namespace se_asset_id_serialize_test;

    for (const HasAssetIds& original : MakeOriginals())
    {
        Array<u8> buffer;
        PackedWriter writer(buffer);
        ASSERT_TRUE(serde::Serialize(writer, original).HasValue());

        PackedReader reader(buffer);
        HasAssetIds result = MakeTarget();
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result, original);
    }
}

TEST(AssetIdSerializeTest, TomlRoundTrip)
{
    using namespace se_asset_id_serialize_test;

    for (const HasAssetIds& original : MakeOriginals())
    {
        toml::table table;
        TomlWriter writer(table);
        ASSERT_TRUE(serde::Serialize(writer, original).HasValue());

        TomlReader reader(table);
        HasAssetIds result = MakeTarget();
        ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
        EXPECT_EQ(result, original);
    }
}

TEST(AssetIdSerializeTest, PackedWritesSameBytesAsGuid)
{
    const Guid guid = Guid::NewGuid();

    Array<u8> asset_id_bytes;
    PackedWriter asset_id_writer(asset_id_bytes);
    ASSERT_TRUE(serde::Serialize(asset_id_writer, AssetId{ guid }).HasValue());

    Array<u8> guid_bytes;
    PackedWriter guid_writer(guid_bytes);
    ASSERT_TRUE(serde::Serialize(guid_writer, guid).HasValue());

    EXPECT_EQ(asset_id_bytes.Len(), sizeof(Guid));
    EXPECT_EQ(asset_id_bytes, guid_bytes);
}

TEST(AssetIdSerializeTest, TomlWritesGuidStrings)
{
    const Guid guid = Guid::NewGuid();
    const se_asset_id_serialize_test::HasAssetIds value{ .id = AssetId{ guid }, .ids = { AssetId::invalid } };

    toml::table table;
    TomlWriter writer(table);
    ASSERT_TRUE(serde::Serialize(writer, value).HasValue());

    const String guid_text = guid.ToString();
    EXPECT_EQ(table["id"].value_exact<std::string>(), guid_text.CStr());
    EXPECT_EQ(table["ids"][0].value_exact<std::string>(), "00000000-0000-0000-0000-000000000000");
}
