#include "Asset/AssetManager.h"


namespace se::asset
{
IAssetLoader* AssetManager::GetLoaderForType(const refl::TypeId& type_id) const
{
    return loaders.FindChecked(type_id).get();
}

refl::TypeId AssetManager::GetTypeFromExtension(const StringName& extension) const
{
    return extension_to_type_map.FindChecked(extension);
}
}
