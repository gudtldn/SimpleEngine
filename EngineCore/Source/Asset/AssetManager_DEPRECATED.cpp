#include "Asset/AssetManager_DEPRECATED.h"
#include "Utility/StringUtils.h"


namespace se::asset
{
Optional<const AssetManager_DEPRECATED::ExtensionInfo&> AssetManager_DEPRECATED::GetExtensionInfo(const StringName& extension) const
{
    return extension_registry.Find(extension);
}

IAssetLoader* AssetManager_DEPRECATED::GetLoaderFromType(const refl::TypeId& type_id) const
{
    constexpr std::unique_ptr<IAssetLoader> null_ptr;
    return loaders.Find(type_id).ValueOr(null_ptr).get();
}

std::shared_ptr<IAssetImportSettings> AssetManager_DEPRECATED::CreateDefaultSettingsForFile(const Path& path) const
{
    const Optional ext_opt = path.Extension();
    if (!ext_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Warning, "Cannot create import settings: file has no extension: {}", path);
        return nullptr;
    }
    const StringName ext_name = ext_opt->CStr();

    // 레지스트리 조회
    if (const Optional info_opt = extension_registry.Find(ext_name))
    {
        const refl::TypeId& settings_type = info_opt->settings_type;
        return CreateSettingsFromType(settings_type);
    }
    return nullptr;
}

std::shared_ptr<IAssetImportSettings> AssetManager_DEPRECATED::CreateSettingsFromType(const refl::TypeId& settings_type) const
{
    if (const Optional settings_ptr_opt = settings_prototypes.Find(settings_type))
    {
        return (*settings_ptr_opt)->Clone();
    }
    return nullptr;
}
}
