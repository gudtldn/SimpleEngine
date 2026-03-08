#include "SimpleEngine/Asset/AssetMetaData.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(AssetDependencyEntry, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(source_vpath, meta::Property)
    SE_REFLECT_PROPERTY(asset_guid, meta::Property)
    SE_REFLECT_PROPERTY(type, meta::Property)
SE_END_REFLECT(AssetDependencyEntry)

SE_BEGIN_REFLECT(SubAssetMeta, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(name, meta::Property)
    SE_REFLECT_PROPERTY(guid, meta::Property)
    SE_REFLECT_PROPERTY(type, meta::Property)
    SE_REFLECT_PROPERTY(dependencies, meta::Property)
SE_END_REFLECT(SubAssetMeta)

SE_BEGIN_REFLECT(AssetMetadata, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(guid, meta::Property)
    SE_REFLECT_PROPERTY(source_hash, meta::Property)
    SE_REFLECT_PROPERTY(source_mtime, meta::Property)
    SE_REFLECT_PROPERTY(source_size, meta::Property)
    SE_REFLECT_PROPERTY(cache_version, meta::Property)
    SE_REFLECT_PROPERTY(sub_assets, meta::Property)
SE_END_REFLECT(AssetMetadata)
}  // namespace se::asset
