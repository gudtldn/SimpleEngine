#include "SimpleEngine/Asset/AssetRegistry.h"


namespace se::asset
{
void AssetRegistry::RegisterAsset(const AssetId& asset_id, const TypeId& asset_type, AssetPath&& asset_path)
{
    std::unique_lock lock(registry_mutex);

    Path file_path = asset_path.GetFilePath();
    String sub_name = asset_path.GetSubAssetName();

    path_to_id.Insert(asset_path, asset_id);
    id_to_path.Insert(asset_id, std::move(asset_path));

    file_to_assets.Emplace(std::move(file_path)).Push({
        .id = asset_id,
        .type = asset_type,
        .sub_name = std::move(sub_name),
    });
}

Optional<const AssetId&> AssetRegistry::GetAssetId(const AssetPath& asset_path) const
{
    std::shared_lock lock(registry_mutex);
    return path_to_id.Find(asset_path);
}

Optional<const AssetPath&> AssetRegistry::GetAssetPath(const AssetId& asset_id) const
{
    std::shared_lock lock(registry_mutex);
    return id_to_path.Find(asset_id);
}

Optional<const AssetId&> AssetRegistry::FindFirstOfType(const Path& file_path, const TypeId& type) const
{
    std::shared_lock lock(registry_mutex);

    if (const Optional entries_opt = file_to_assets.Find(file_path))
    {
        for (const AssetEntry& entry : *entries_opt)
        {
            if (entry.type == type)
            {
                return entry.id;
            }
        }
    }
    return std::nullopt;
}

Optional<const Array<AssetEntry>&> AssetRegistry::GetAssetsInFile(const Path& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return file_to_assets.Find(file_path);
}

void AssetRegistry::MarkFileAsImported(const Path& file_path)
{
    std::unique_lock lock(registry_mutex);
    imported_files.Insert(file_path);
}

bool AssetRegistry::IsFileImported(const Path& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return imported_files.Contains(file_path);
}
}
