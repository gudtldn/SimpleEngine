#include "SimpleEngine/Asset/AssetMetaData.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"
#include "SimpleEngine/Core/Types/StringName.h"


SE_REFLECT_ENUM_BEGIN(se::EAssetDependencyType)
    SE_ENUM_VALUE(Hard)
    SE_ENUM_VALUE(Soft)
    SE_ENUM_VALUE(BuildOnly)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se::AssetDependencyEntry, se::display::Hidden)
    SE_FIELD(source_vpath)
    SE_FIELD(asset_guid)
    SE_FIELD(type)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::SubAssetMeta, se::display::Hidden)
    SE_FIELD(name)
    SE_FIELD(guid)
    SE_FIELD(type)
    SE_FIELD(dependencies)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::AssetMetadata, se::display::Hidden)
    SE_FIELD(guid)
    SE_FIELD(source_hash)
    SE_FIELD(source_mtime)
    SE_FIELD(source_size)
    SE_FIELD(cache_version)
    SE_FIELD(settings_hash)
    SE_FIELD(sub_assets)
SE_REFLECT_END()
