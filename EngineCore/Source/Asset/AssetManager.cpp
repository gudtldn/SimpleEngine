#include "Asset/AssetManager.h"
#include "Utility/StringUtils.h"


namespace se::asset
{
IAssetLoader* AssetManager::GetLoaderForType(const refl::TypeId& type_id) const
{
    return loaders.Find(type_id).ValueOr(nullptr).get();
}

std::shared_ptr<IAssetImportSettings> AssetManager::GetSettingsForType(const refl::TypeId& type_id) const
{
    if (Optional factory_opt = settings_factories.Find(type_id))
    {
        return (*factory_opt)();
    }
    return nullptr;
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
