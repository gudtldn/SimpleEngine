#include "SimpleEditor/Asset/Pipeline/v2/ImportResult.h"


namespace se::editor::v2
{
ImportResult::ImportResult(
    Array<ImportedAsset> in_entries,
    HashMap<String, uint32> in_name_to_index,
    uint32 in_main_asset_index
)
    : entries(std::move(in_entries))
    , name_to_index(std::move(in_name_to_index))
    , main_asset_index(in_main_asset_index)
{
}

const ImportedAsset* ImportResult::GetMainAsset() const
{
    if (entries.IsEmpty())
    {
        return nullptr;
    }
    return &entries[main_asset_index];
}

const ImportedAsset* ImportResult::FindByName(StringView name) const
{
    if (const Optional idx = name_to_index.Find(String(name)))
    {
        return &entries[*idx];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

uint32 ImportResult::Builder::RegisterAsset(
    const String& name,
    AssetId asset_id,
    std::shared_ptr<AssetBase> asset,
    Array<AssetDependencyEntry> dependencies
)
{
    const String unique_name = MakeUniqueName(name);
    const auto index = static_cast<uint32>(entries.Len());

    entries.Push({
        .name         = unique_name,
        .asset_id     = asset_id,
        .asset        = std::move(asset),
        .dependencies = std::move(dependencies),
    });
    name_to_index.Insert(unique_name, index);
    return index;
}

void ImportResult::Builder::SetMainAssetIndex(uint32 index)
{
    main_asset_index = index;
}

ImportResult ImportResult::Builder::Build()
{
    ImportResult result{
        std::move(entries),
        std::move(name_to_index),
        main_asset_index,
    };

    entries = {};
    name_to_index = {};
    next_suffix_map = {};
    main_asset_index = 0;

    return result;
}

String ImportResult::Builder::MakeUniqueName(const String& base_name)
{
    if (!name_to_index.Contains(base_name))
    {
        return base_name;
    }

    uint32& suffix = next_suffix_map[base_name];
    ++suffix;

    String candidate;
    do
    {
        candidate = String::Format("{}_{}", base_name, suffix);
        ++suffix;
    }
    while (name_to_index.Contains(candidate));

    return candidate;
}
} // namespace se::editor::v2