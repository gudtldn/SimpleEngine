#include "Asset/AssetManager.h"
#include "Utility/StringUtils.h"


namespace se::asset
{
Optional<const AssetManager::ExtensionInfo&> AssetManager::GetExtensionInfo(const StringName& extension) const
{
    return extension_registry.Find(extension);
}

IAssetLoader* AssetManager::GetLoaderFromType(const refl::TypeId& type_id) const
{
    constexpr std::unique_ptr<IAssetLoader> null_ptr;
    return loaders.Find(type_id).ValueOr(null_ptr).get();
}

std::shared_ptr<IAssetImportSettings> AssetManager::CreateDefaultSettingsForFile(const std::filesystem::path& path) const
{
    const StringName ext_name = utility::ToString(path.extension().c_str());

    // 레지스트리 조회
    if (const Optional info_opt = extension_registry.Find(ext_name))
    {
        const refl::TypeId& settings_type = info_opt->settings_type;
        return CreateSettingsFromType(settings_type);
    }
    return nullptr;
}

std::shared_ptr<IAssetImportSettings> AssetManager::CreateSettingsFromType(const refl::TypeId& settings_type) const
{
    if (const Optional settings_ptr_opt = settings_prototypes.Find(settings_type))
    {
        return (*settings_ptr_opt)->Clone();
    }
    return nullptr;
}
}
