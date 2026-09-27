#include "SimpleEngine/Asset/AssetMetaData.h"
#include "../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se
{
/**
 * 바이너리는 타입 이름의 해시, 텍스트는 타입 이름으로 씁니다. 레거시 archive와 같은 표현입니다.
 * 등록되지 않은 타입이면 레거시 TypeId_v1::FromHash, FromName처럼 빈 TypeId로 읽습니다.
 */
template <>
struct SerializeTraits<TypeId_v1>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const TypeId_v1& value)
    {
        if (writer.IsTextFormat())
        {
            writer.Str(value.GetName());
            return;
        }
        writer.Int(static_cast<i64>(value.GetHash()), serde::IntWidthOf<u64>(), false);
    }

    static void Read(ArchiveReader& reader, TypeId_v1& value)
    {
        if (reader.IsTextFormat())
        {
            String name;
            reader.Str(name);
            if (!reader.HasError())
            {
                value = TypeId_v1::FromName(StringName{ name });
            }
            return;
        }

        i64 hash = 0;
        reader.Int(hash, serde::IntWidthOf<u64>(), false);
        if (!reader.HasError())
        {
            value = TypeId_v1::FromHash(static_cast<u64>(hash));
        }
    }
};

SE_BEGIN_REFLECT_V1(AssetDependencyEntry, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(source_vpath, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(asset_guid, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(type, meta::Reflect)
SE_END_REFLECT_V1(AssetDependencyEntry)

SE_BEGIN_REFLECT_V1(SubAssetMeta, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(name, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(guid, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(type, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(dependencies, meta::Reflect)
SE_END_REFLECT_V1(SubAssetMeta)

SE_BEGIN_REFLECT_V1(AssetMetadata, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(guid, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(source_hash, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(source_mtime, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(source_size, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(cache_version, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(settings_hash, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(sub_assets, meta::Reflect)
SE_END_REFLECT_V1(AssetMetadata)
} // namespace se

SE_REGISTER_SERIALIZE_TRAITS(se::TypeId_v1)


SE_REFLECT_ENUM_BEGIN(se::EAssetDependencyType)
    SE_ENUM_VALUE(Hard)
    SE_ENUM_VALUE(Soft)
    SE_ENUM_VALUE(BuildOnly)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se::AssetDependencyEntry)
    SE_FIELD(source_vpath)
    SE_FIELD(asset_guid)
    SE_FIELD(type)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::SubAssetMeta)
    SE_FIELD(name)
    SE_FIELD(guid)
    SE_FIELD(type)
    SE_FIELD(dependencies)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::AssetMetadata)
    SE_FIELD(guid)
    SE_FIELD(source_hash)
    SE_FIELD(source_mtime)
    SE_FIELD(source_size)
    SE_FIELD(cache_version)
    SE_FIELD(settings_hash)
    SE_FIELD(sub_assets)
SE_REFLECT_END()
