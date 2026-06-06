#include "SimpleEngine/Asset/AssetMetaData.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT(AssetDependencyEntry, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(source_vpath, meta::Reflect)
    SE_REFLECT_PROPERTY(asset_guid, meta::Reflect)
    SE_REFLECT_PROPERTY(type, meta::Reflect)
SE_END_REFLECT(AssetDependencyEntry)

SE_BEGIN_REFLECT(SubAssetMeta, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(name, meta::Reflect)
    SE_REFLECT_PROPERTY(guid, meta::Reflect)
    SE_REFLECT_PROPERTY(type, meta::Reflect)
    SE_REFLECT_PROPERTY(dependencies, meta::Reflect)
SE_END_REFLECT(SubAssetMeta)

SE_BEGIN_REFLECT(AssetMetadata, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(guid, meta::Reflect)
    SE_REFLECT_PROPERTY(source_hash, meta::Reflect)
    SE_REFLECT_PROPERTY(source_mtime, meta::Reflect)
    SE_REFLECT_PROPERTY(source_size, meta::Reflect)
    SE_REFLECT_PROPERTY(cache_version, meta::Reflect)
    SE_REFLECT_PROPERTY(settings_hash, meta::Reflect)
    SE_REFLECT_PROPERTY(sub_assets, meta::Reflect)
SE_END_REFLECT(AssetMetadata)
} // namespace se
