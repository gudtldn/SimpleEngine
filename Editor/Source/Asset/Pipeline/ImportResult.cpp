#include "SimpleEditor/Asset/Pipeline/ImportResult.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Debug.h"


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
    const String& unique_name,
    AssetId asset_id,
    std::shared_ptr<AssetBase> asset,
    Array<AssetDependencyEntry> dependencies
)
{
    // 여기서 중복이 감지되면 어떤 Translator가 ImportContext::AllocateSubAsset을 우회했다는 상류 버그 신호.
    SE_ENSURE(!name_to_index.Contains(unique_name),
        "Duplicate sub-asset name reached ImportResult (translator bypassed ImportContext::AllocateSubAsset?): {}", unique_name);

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
    return {
        std::exchange(entries, {}),
        std::exchange(name_to_index, {}),
        std::exchange(main_asset_index, 0)
    };
}
} // namespace se::editor
