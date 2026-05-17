#include "SimpleEditor/Asset/Pipeline/ImportResult.h"


namespace se::editor
{
ImportResult::ImportResult(
    Array<ImportedAsset> in_entries,
    HashMap<String, u32> in_name_to_index,
    u32 in_main_asset_index
)
    : entries(std::move(in_entries))
    , name_to_index(std::move(in_name_to_index))
    , main_asset_index(in_main_asset_index)
{
}

Optional<const ImportedAsset&> ImportResult::GetMainAsset() const
{
    if (main_asset_index < entries.Len())
    {
        return entries[main_asset_index];
    }
    return NullOpt;
}

Optional<const ImportedAsset&> ImportResult::FindByName(StringView name) const
{
    if (const auto idx = name_to_index.Find(name))
    {
        return entries[*idx];
    }
    return NullOpt;
}

u32 ImportResult::Builder::RegisterAsset(
    const String& name,
    AssetId asset_id,
    std::shared_ptr<AssetBase> asset,
    Array<AssetDependencyEntry> dependencies
)
{
    const String unique_name = MakeUniqueName(name);
    const auto index = static_cast<u32>(entries.Len());

    entries.Push({
        .name = unique_name,
        .asset_id = asset_id,
        .asset = std::move(asset),
        .dependencies = std::move(dependencies),
    });
    name_to_index.Insert(unique_name, index);
    return index;
}

void ImportResult::Builder::SetMainAssetIndex(u32 index)
{
    main_asset_index = index;
}

ImportResult ImportResult::Builder::Build()
{
    ImportResult result = {
        std::exchange(entries, {}),
        std::exchange(name_to_index, {}),
        std::exchange(main_asset_index, 0)
    };

    next_suffix_map = {};
    return result;
}

String ImportResult::Builder::MakeUniqueName(const String& base_name)
{
    // 이름이 이미 존재하지 않으면 그대로 반환
    if (!name_to_index.Contains(base_name))
    {
        return base_name;
    }

    // 중복 시 suffix 추가: Name_1, Name_2, ...
    u32& suffix = next_suffix_map.Entry(base_name).OrDefault();
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
} // namespace se::editor
