#include "SimpleEngine/Asset/AssetMetaData.h"
#include "../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se
{
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
