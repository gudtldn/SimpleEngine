#include "Asset/AssetManager.h"
#include "Utility/StringUtils.h"


namespace se::asset
{
IAssetLoader* AssetManager::GetLoaderForType(const refl::TypeId& type_id) const
{
    return loaders.Find(type_id).ValueOr(nullptr).get();
}

Optional<const refl::TypeId&> AssetManager::GetTypeFromExtension(const std::filesystem::path& extension) const
{
    SE_ASSERT(!extension.empty());
    return GetTypeFromExtension(utility::string::ToString(extension.c_str()));
}

Optional<const refl::TypeId&> AssetManager::GetTypeFromExtension(const StringName& extension) const
{
    return extension_to_type_map.Find(extension);
}
}
